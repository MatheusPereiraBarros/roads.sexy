#include "xodr_viewer_window.h"

#include <QtGui/QPainter>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QListWidgetItem>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QScrollArea>
#include <iosfwd>

#include "perlin_noise.h"

#include <string>
#include <fstream>
#include <limits>
#include <cmath>

#include "bounding_rect.h"
#include "xodr/xodr_map.h"

namespace aid {
    namespace xodr {

        static constexpr float DRAW_SCALE = 6 * 2;
        static constexpr float DRAW_MARGIN = 200;

        struct XodrFileInfo {
            const char *name;
            const char *path;
        };

        static const XodrFileInfo xodrFiles[] = {
                {"Crossing8Course",   "data/opendrive/Crossing8Course.xodr"},
                {"CulDeSac",          "data/opendrive/CulDeSac.xodr"},
                {"Roundabout8Course", "data/opendrive/Roundabout8Course.xodr"},
                {"sample1.1",         "data/opendrive/sample1.1.xodr"},
        };

/**
 * @brief The view which is shown inside the main area's QScrollArea.
 *
 * This view renders the XodrMap specified using the setMap function.
 */
        class XodrViewerWindow::XodrView : public QWidget {
        public:
            /**
             * @brief Constructs a new XodrView.
             *
             * @param parent        The parent widget.
             */
            XodrView(QWidget *parent = nullptr) : QWidget(parent) {}

            void setMap(std::unique_ptr<XodrMap> &&xodrMap);

            virtual void paintEvent(QPaintEvent *evnt) override;

            virtual void getObjFile();

        private:
            /**
             * @brief Converts a point form XODR map coordinates to view coordinates.
             *
             * @param pt            The point in map coordinates.
             * @return              The point in view coordinates.
             */
            QPointF pointMapToView(const Eigen::Vector2d pt) const;

            std::unique_ptr<XodrMap> xodrMap_;

            /**
             * @brief The offset used in the pointMapToView function.
             *
             * See the pointMapToView function for the exact meaning of it.
             */
            Eigen::Vector2d mapToViewOffset_;
        };

        XodrViewerWindow::XodrViewerWindow() {
            setWindowTitle("XODR Viewer");
            resize(1200, 900);

            sideBar_ = new QListWidget();
            for (const XodrFileInfo &fileInfo : xodrFiles) {
                sideBar_->addItem(fileInfo.name);
            }

            QDockWidget *sideBarDockWidget = new QDockWidget();
            sideBarDockWidget->setWindowTitle("XODR Files");
            sideBarDockWidget->setContentsMargins(0, 0, 0, 0);
            sideBarDockWidget->setWidget(sideBar_);
            addDockWidget(Qt::DockWidgetArea::LeftDockWidgetArea, sideBarDockWidget);

            QScrollArea *scrollArea = new QScrollArea();
            setCentralWidget(scrollArea);

            xodrView_ = new XodrView();
            scrollArea->setWidget(xodrView_);

            QObject::connect(sideBar_, &QListWidget::currentRowChanged, this, &XodrViewerWindow::onXodrFileSelected);
        }

        void XodrViewerWindow::onXodrFileSelected(int index) {
            const char *path = xodrFiles[index].path;

            std::cout << "Loading xodr file: " << path << std::endl;

            XodrParseResult <XodrMap> fromFileRes = XodrMap::fromFile(path);

            if (!fromFileRes.hasFatalErrors()) {
                std::unique_ptr<XodrMap> xodrMap(new XodrMap(std::move(fromFileRes.value())));
                xodrView_->setMap(std::move(xodrMap));
            } else {
                std::cout << "Errors: " << std::endl;
                for (const auto &err : fromFileRes.errors()) {
                    std::cout << err.description() << std::endl;
                }

                QMessageBox::critical(this, "XODR Viewer", QString("Failed to load xodr file %1.").arg(path));
            }
        }

        void XodrViewerWindow::XodrView::setMap(std::unique_ptr<XodrMap> &&xodrMap) {
            xodrMap_ = std::move(xodrMap);

            BoundingRect boundingRect = xodrMapApproxBoundingRect(*xodrMap_);

            Eigen::Vector2d diag = boundingRect.max_ - boundingRect.min_;

            // Compute the size big enough for the bounding rectangle, scaled by
            // DRAW_SCALE, and with margins of size DRAW_MARGIN on all sides.
            QSize size(static_cast<int>(std::ceil(diag.x() * DRAW_SCALE + 2 * DRAW_MARGIN)),
                       static_cast<int>(std::ceil(diag.y() * DRAW_SCALE + 2 * DRAW_MARGIN)));
            resize(size);

            mapToViewOffset_ = Eigen::Vector2d(-boundingRect.min_.x() * DRAW_SCALE + DRAW_MARGIN,
                                               boundingRect.max_.y() * DRAW_SCALE + DRAW_MARGIN);
        }

        static bool showLaneType(LaneType laneType) {
            return laneType == LaneType::DRIVING ||
                   laneType == LaneType::SIDEWALK ||
                   laneType == LaneType::BORDER;
        }


        constexpr double driving_elevation = 0.2;
        constexpr double sidewalk_elevation = 0.4;
        constexpr double border_elevation = 0.45;
        constexpr double road_markings_shift = 0.2;

        constexpr double road_markings_width = 0.25;
        constexpr double road_markings_elevation = 0.25;
        constexpr int road_markings_stripe_length = 3;
        constexpr int road_markings_stripe_distance = 3;

        constexpr double padding = 300;
        constexpr int noiseScale = 5;
        constexpr int noiseHeight = 20;
        constexpr int numPoints = 257;


        double getHeight(double x, double y, double minY, double minX, double width) {
            static siv::PerlinNoise noise(32);
            double n = noise.noise((x - minX) / width * noiseScale, (y - minY) / width * noiseScale);
            double x_mid = minX + width / 2;
            double y_mid = minY + width / 2;
            double dist = sqrt((x - x_mid) * (x - x_mid) + (y - y_mid) * (y - y_mid));
            n *= pow((1 - cos(dist * 3.14 / (width / 2))) / 2, 1.1) + 0.4;
            return n * noiseHeight + noiseHeight;
        }

        LaneSection::BoundaryCurveTessellation shift(
                const LaneSection::BoundaryCurveTessellation &original,
                const LaneSection::BoundaryCurveTessellation &ref,
                double shift,
                int from = 0, int to = -1) {

            std::vector<Eigen::Vector2d> shifted_vertices;
            if (to == -1) to = original.vertices_.size();
            for (int i = from; i < to; i++) {
                Eigen::Vector2d pt_orig = original.vertices_[i];
                Eigen::Vector2d pt_ref = ref.vertices_[i];
                shifted_vertices.push_back((pt_ref - pt_orig).normalized() * shift + pt_orig);
            }
            return LaneSection::BoundaryCurveTessellation{std::move(shifted_vertices)};
        }

        std::stringstream writeStreet(
                const LaneSection::BoundaryCurveTessellation &b,
                const LaneSection::BoundaryCurveTessellation &a, double minY, double minX, double width,
                int &index_offset, double elevation) {
            std::stringstream res;
            static int seg_num = 0;

            int size = a.vertices_.size();
            res << "o segment_num_" << seg_num++ << std::endl;


            for (int j = 0; j < size; j++) {
                Eigen::Vector2d ptl = a.vertices_[j];
                Eigen::Vector2d ptlr = b.vertices_[j];
                res << "v " << ptl.x() << " " << ptl.y() << " "
                    << getHeight(ptl.x(), ptl.y(), minX, minY, width) + elevation << std::endl;
                res << "v " << ptlr.x() << " " << ptlr.y() << " "
                    << getHeight(ptlr.x(), ptlr.y(), minX, minY, width) + elevation << std::endl;
            }
            for (int j = 0; j < size; j++) {
                Eigen::Vector2d ptl = a.vertices_[j];
                Eigen::Vector2d ptlr = b.vertices_[j];
                res << "v " << ptl.x() << " " << ptl.y() << " 0" << std::endl;
                res << "v " << ptlr.x() << " " << ptlr.y() << " 0" << std::endl;
            }

            size *= 2;
            // triangles = road surface
            for (int j = 1; j <= size - 2; j++) {
                if (j % 2 == 0) {
                    res << "f " << j + index_offset << " " << j + 1 + index_offset
                        << " " << j + 2 + index_offset << std::endl;
                } else {
                    res << "f " << j + index_offset << " " << j + 2 + index_offset
                        << " " << j + 1 + index_offset << std::endl;
                }
            }
            // sides
            for (int j = 1; j <= size - 2; j++) {
                if (j % 2 == 1) {
                    res << "f " << j + index_offset << " " << j + size + index_offset
                        << " " << j + 2 + size + index_offset << std::endl;
                    res << "f " << j + index_offset
                        << " " << j + 2 + size + index_offset << " "
                        << j + 2 + index_offset << std::endl;
                } else {
                    res << "f " << j + 2 + index_offset << " " << j + 2 + size + index_offset
                        << " " << j + size + index_offset << std::endl;
                    res << "f " << j + index_offset
                        << " " << j + 2 + index_offset << " "
                        << j + size + index_offset << std::endl;
                }
            }
            res << "f " << 1 + index_offset << " " << 1 + size + index_offset << " "
                << 2 + size + index_offset << " " << 2 + index_offset << std::endl;

            res << "f " << size + index_offset << " " << size + size + index_offset
                << " " << size + size - 1 + index_offset << " "
                << size - 1 + index_offset << std::endl;
            index_offset += size * 2;
            return res;
        }

        std::stringstream writeOrientation(
                const LaneSection::BoundaryCurveTessellation &outer,
                const LaneSection::BoundaryCurveTessellation &inner, double minY, double minX, double width,
                double elevation) {
            static int seg_num = 0;
            std::stringstream res;
            for (int j = 0; j < outer.vertices_.size(); j++) {

                res << "o vec_num_" << seg_num++ << std::endl;
                Eigen::Vector2d ptl = outer.vertices_[j];
                Eigen::Vector2d ptlr = inner.vertices_[j];
                res << "v " << ptl.x() << " " << ptl.y() << " "
                    << getHeight(ptl.x(), ptl.y(), minX, minY, width) + elevation << std::endl;
                res << "v " << ptlr.x() << " " << ptlr.y() << " "
                    << getHeight(ptlr.x(), ptlr.y(), minX, minY, width) + elevation << std::endl;
            }
            return res;

        }

        std::stringstream writeOrientationParallel(
                const LaneSection::BoundaryCurveTessellation &left,
                const LaneSection::BoundaryCurveTessellation &right, double minY, double minX, double width,
                double elevation) {
            static int seg_num = 0;
            std::stringstream res;
            for (int j = 1; j < left.vertices_.size() - 1; j++) {
                res << "o vec_num_" << seg_num++ << std::endl;
                Eigen::Vector2d ptl = left.vertices_[j];
                Eigen::Vector2d ptlPrev = left.vertices_[j - 1];
                Eigen::Vector2d ptlNext = left.vertices_[j + 1];
                Eigen::Vector2d ptlr = right.vertices_[j];

                Eigen::Vector2d mid = (ptl + ptlr) / 2;
                Eigen::Vector2d dir = mid + (ptlNext - ptlPrev).normalized();


                res << "v " << mid.x() << " " << mid.y() << " "
                    << getHeight(mid.x(), mid.y(), minX, minY, width) + elevation << std::endl;
                res << "v " << ptlr.x() << " " << ptlr.y() << " "
                    << getHeight(dir.x(), dir.y(), minX, minY, width) + elevation << std::endl;
            }
            return res;

        }

        void XodrViewerWindow::XodrView::getObjFile() {

            int streets_off = 0;
            std::ofstream streets;
            streets.open("./out/streets.obj");

            int sidewalk_off = 0;
            std::ofstream sidewalk;
            sidewalk.open("./out/sidewalk.obj");

            int border_off = 0;
            std::ofstream border;
            border.open("./out/border.obj");

            int markings_off = 0;
            std::ofstream markings;
            markings.open("./out/markings.obj");

            int terrain_off = 0;
            std::ofstream terrain;
            terrain.open("./out/terrain.obj");

            int all_off = 0;
            std::ofstream all;
            all.open("./out/all.obj");

            std::ofstream street_geo;
            street_geo.open("./out/street_geo.obj");

            std::ofstream street_lanes;
            street_lanes.open("./out/street_lanes.obj");

            std::ofstream terrain_hm;
            terrain_hm.open("./out/terrain.raw", std::ios::out | std::ios::binary);


            double minX = std::numeric_limits<double>::infinity();
            double maxX = std::numeric_limits<double>::lowest();
            double minY = std::numeric_limits<double>::infinity();
            double maxY = std::numeric_limits<double>::lowest();

            struct DrawLane {
                LaneSection::BoundaryCurveTessellation left;
                LaneSection::BoundaryCurveTessellation right;
                double elevation;
            };

            std::vector<DrawLane> lanes_to_draw;
            std::vector<DrawLane> lanes_to_draw_sidewalk;
            std::vector<DrawLane> lanes_to_draw_boundary;
            std::vector<DrawLane> lanes_to_draw_markings;
            std::vector<DrawLane> geometry;
            std::vector<DrawLane> lane_directions;



            // roads
            for (const Road &road : xodrMap_->roads()) {
                const auto &laneSections = road.laneSections();

                // find left and rig

                // road section
                for (int laneSectionIdx = 0; laneSectionIdx < (int) laneSections.size(); laneSectionIdx++) {
                    const LaneSection &laneSection = laneSections[laneSectionIdx];

                    auto refLineTessellation = road.referenceLine().tessellate(laneSection.startS(),
                                                                               laneSection.endS());
                    auto boundaries = laneSection.tessellateLaneBoundaryCurves(refLineTessellation);
                    const auto &lanes = laneSection.lanes();

                    for (size_t i = 0; i < boundaries.size() - 1; i++) {
                        const LaneSection::BoundaryCurveTessellation &left = boundaries[i];
                        const LaneSection::BoundaryCurveTessellation &right = boundaries[i + 1];

                        if (lanes[i].type() == LaneType::DRIVING) {

                            DrawLane drawLane{left, right, driving_elevation};
                            lanes_to_draw.push_back(std::move(drawLane));

                            if (i < boundaries.size() / 2) {
                                lane_directions.push_back({right, left, sidewalk_elevation});
                            } else {
                                lane_directions.push_back({left, right, sidewalk_elevation});
                            }

                            if (i > 0 && (lanes[i - 1].type() == LaneType::BORDER ||
                                          lanes[i - 1].type() == LaneType::SHOULDER) &&
                                (i < 2 || lanes[i - 2].type() == LaneType::SIDEWALK)) {
                                // Shift right of i
                                auto l = shift(left, right, road_markings_shift);
                                auto r = shift(left, right, road_markings_shift + road_markings_width);
                                DrawLane drawLane{std::move(l), std::move(r), road_markings_elevation};
                                lanes_to_draw_markings.push_back(drawLane);
                            } else if ((lanes[i + 1].type() == LaneType::BORDER ||
                                        lanes[i + 1].type() == LaneType::SHOULDER) &&
                                       (i > boundaries.size() - 2 || lanes[i + 2].type() == LaneType::SIDEWALK)) {
                                auto l = shift(right, left, road_markings_shift);
                                auto r = shift(right, left, road_markings_shift + road_markings_width);
                                DrawLane drawLane{std::move(r), std::move(l), road_markings_elevation};
                                lanes_to_draw_markings.push_back(drawLane);
                            }
                            if (i > 0 && lanes[i - 1].type() == LaneType::DRIVING) {
                                for (int k = 0; k < left.vertices_.size() -
                                                    road_markings_stripe_length; k += road_markings_stripe_length +
                                                                                      road_markings_stripe_distance) {
                                    auto l = shift(left, right, -road_markings_width / 2, k,
                                                   k + road_markings_stripe_length);
                                    auto r = shift(left, right, road_markings_width / 2, k,
                                                   k + road_markings_stripe_length);
                                    DrawLane drawLane{std::move(l), std::move(r), road_markings_elevation};
                                    lanes_to_draw_markings.push_back(drawLane);
                                }

                            }
                        } else if (lanes[i].type() == LaneType::SIDEWALK) {
                            DrawLane drawLane{left, right, sidewalk_elevation};
                            lanes_to_draw_sidewalk.push_back(std::move(drawLane));

                            if (i < boundaries.size() / 2) {
                                geometry.push_back({right, left, sidewalk_elevation});
                            } else {
                                geometry.push_back({left, right, sidewalk_elevation});
                            }
                        } else if (lanes[i].type() == LaneType::BORDER) {
                            if ((i < 1 || lanes[i - 1].type() != LaneType::SIDEWALK) &&
                                (i > boundaries.size() - 1 || lanes[i + 1].type() != LaneType::SIDEWALK)) {
                                continue;
                            }
                            DrawLane drawLane{left, right, border_elevation};
                            lanes_to_draw_boundary.push_back(std::move(drawLane));
                        } else {
                            continue;
                        }

                        for (int j = 0; j < left.vertices_.size(); j++) {
                            Eigen::Vector2d ptl = left.vertices_[j];
                            Eigen::Vector2d ptlr = right.vertices_[j];
                            minX = std::min(minX, ptl.x());
                            maxX = std::max(maxX, ptl.x());
                            minY = std::min(minY, ptl.y());
                            maxY = std::max(maxY, ptl.y());

                            minX = std::min(minX, ptlr.x());
                            maxX = std::max(maxX, ptlr.x());
                            minY = std::min(minY, ptlr.y());
                            maxY = std::max(maxY, ptlr.y());
                        }
                    }
                };

            }

            // terrain
            double deltaX = maxX - minX;
            double deltaY = maxY - minY;

            double width;

            if (deltaX > deltaY) {
                minY -= (deltaX - deltaY) / 2;
                maxY += (deltaX - deltaY) / 2;
                width = deltaX + 2 * padding;
            } else {
                minX -= (deltaY - deltaX) / 2;
                maxX += (deltaY - deltaX) / 2;
                width = deltaY + 2 * padding;
            }

            minX -=
                    padding;
            maxX +=
                    padding;
            minY -=
                    padding;
            maxY +=
                    padding;


// roads

            for (const DrawLane &drawLane: lanes_to_draw) {
                std::stringstream write =
                        writeStreet(drawLane.left, drawLane.right, minY, minX, width, all_off, drawLane.elevation);
                all << write.rdbuf();

                write = writeStreet(drawLane.left, drawLane.right, minY, minX, width, streets_off, drawLane.elevation);
                streets << write.rdbuf();
            }
            for (const DrawLane &drawLane: lanes_to_draw_boundary) {
                std::stringstream write =
                        writeStreet(drawLane.left, drawLane.right, minY, minX, width, all_off, drawLane.elevation);
                all << write.rdbuf();

                write = writeStreet(drawLane.left, drawLane.right, minY, minX, width, border_off, drawLane.elevation);
                border << write.rdbuf();
            }
            for (const DrawLane &drawLane: lanes_to_draw_markings) {
                std::stringstream write =
                        writeStreet(drawLane.left, drawLane.right, minY, minX, width, all_off, drawLane.elevation);
                all << write.rdbuf();

                write = writeStreet(drawLane.left, drawLane.right, minY, minX, width, markings_off, drawLane.elevation);
                markings << write.rdbuf();
            }
            for (const DrawLane &drawLane: lanes_to_draw_sidewalk) {
                std::stringstream write =
                        writeStreet(drawLane.left, drawLane.right, minY, minX, width, all_off, drawLane.elevation);
                all << write.rdbuf();

                write = writeStreet(drawLane.left, drawLane.right, minY, minX, width, sidewalk_off, drawLane.elevation);
                sidewalk << write.rdbuf();
            }
            for (const DrawLane &drawLane : geometry) {
                street_geo << writeOrientation(drawLane.left, drawLane.right, minY, minX, width,
                                               drawLane.elevation).rdbuf();
            }
            for (const DrawLane &drawLane : lane_directions) {
                street_lanes << writeOrientationParallel(drawLane.left, drawLane.right, minY, minX, width,
                                                         drawLane.elevation).rdbuf();
            }

            double delta = width / (numPoints - 1);


            terrain << "o terrain" <<
                    std::endl;
            all << "o terrain" <<
                std::endl;

            for (
                    int c = 0;
                    c < numPoints;
                    c++) {
                for (
                        int r = 0;
                        r < numPoints;
                        r++) {
                    double x = minX + c * delta;
                    double y = minY + r * delta;
                    double z = getHeight(x, y, minX, minY, width);
                    unsigned short z_discrete = static_cast<short>(z / (2 * noiseHeight) * 8192);
                    terrain_hm.write((char *) &z_discrete, sizeof(z_discrete));
                    terrain << "v " << x << " " << y << " " << z <<
                            std::endl;
                    all << "v " << x << " " << y << " " << z <<
                        std::endl;

                }
            }

            for (
                    int c = 0;
                    c < numPoints - 1; c++) {
                for (
                        int r = 0;
                        r < numPoints - 1; r++) {
                    int startNum = 1 + c + r * numPoints;
                    terrain << "f " << startNum + numPoints << " " << startNum + numPoints + 1 << " "
                            << startNum <<
                            std::endl;
                    terrain << "f " << startNum + numPoints + 1 << " " << startNum + 1 << " "
                            << startNum <<
                            std::endl;

                    all << "f " << startNum + numPoints + all_off << " " << startNum + all_off + numPoints + 1 << " "
                        << startNum + all_off <<
                        std::endl;
                    all << "f " << startNum + numPoints + 1 + all_off << " " << startNum + 1 + all_off << " "
                        << startNum + all_off <<
                        std::endl;
                }
            }
            streets.close();
            border.close();
            sidewalk.close();
            markings.close();
            terrain.close();
            terrain_hm.close();
            all.close();
            street_geo.close();
            street_lanes.close();

            std::cout << "Finished writing file." << std::endl <<
                      std::flush;
        }

        void XodrViewerWindow::XodrView::paintEvent(QPaintEvent *) {
            QPainter painter(this);

            getObjFile();

            QVector <QPointF> allPoints;

            // roads
            for (const Road &road : xodrMap_->roads()) {
                const auto &laneSections = road.laneSections();

                // debug line highlighting
                for (int laneSectionIdx = 0; laneSectionIdx < (int) laneSections.size(); laneSectionIdx++) {
                    const LaneSection &laneSection = laneSections[laneSectionIdx];

                    auto refLineTessellation = road.referenceLine().tessellate(laneSection.startS(),
                                                                               laneSection.endS());
                    auto boundaries = laneSection.tessellateLaneBoundaryCurves(refLineTessellation);
                    const auto &lanes = laneSection.lanes();

                    for (size_t i = 0; i < boundaries.size(); i++) {
                        if (lanes[i].type() == LaneType::DRIVING) {
                            painter.setPen(QPen(Qt::yellow, 4, Qt::SolidLine, Qt::RoundCap));
                        } else if (lanes[i].type() == LaneType::SIDEWALK) {
                            painter.setPen(QPen(Qt::cyan, 4, Qt::SolidLine, Qt::RoundCap));
                        } else if (lanes[i].type() == LaneType::NONE) {
                            painter.setPen(QPen(Qt::red, 4, Qt::SolidLine, Qt::RoundCap));
                        } else if (lanes[i].type() == LaneType::SHOULDER) {
                            painter.setPen(QPen(Qt::magenta, 4, Qt::SolidLine, Qt::RoundCap));
                        } else {
                            painter.setPen(QPen(Qt::lightGray, 1, Qt::SolidLine, Qt::RoundCap));
                        }

                        const LaneSection::BoundaryCurveTessellation &boundary = boundaries[i];
                        QVector <QPointF> points;
                        for (Eigen::Vector2d pt : boundary.vertices_) {
                            points.append(pointMapToView(pt));
                        }

                        painter.drawPolyline(points);
                    }
                }


                // find left and rig
                // road section
                for (
                        int laneSectionIdx = 0;

                        laneSectionIdx < (int)

                                laneSections.

                                        size();

                        laneSectionIdx++) {
                    const LaneSection &laneSection = laneSections[laneSectionIdx];

                    auto refLineTessellation = road.referenceLine().tessellate(laneSection.startS(),
                                                                               laneSection.endS());
                    auto boundaries = laneSection.tessellateLaneBoundaryCurves(refLineTessellation);
                    const auto &lanes = laneSection.lanes();

                    // lanes
                    // Find first and last lane
                    int left = -1;
                    int right = -1;
                    for (
                            size_t i = 0;
                            i < boundaries.

                                    size();

                            i++) {
                        const LaneSection::BoundaryCurveTessellation &boundary = boundaries[i];
                        if (lanes[i].

                                type()

                            == LaneType::BORDER || lanes[i].

                                type()

                                                   == LaneType::DRIVING) {
                            if (left == -1) {
                                left = i;
                            }
                            right = i;
                        }
                    }
                    if (left == -1 || right == -1) continue;
                    const LaneSection::BoundaryCurveTessellation &leftBoundary = boundaries[left];
                    const LaneSection::BoundaryCurveTessellation &rightBoundary = boundaries[right];
                    QVector <QPointF> leftPoints;
                    QVector <QPointF> rightPoints;
                    for (
                        Eigen::Vector2d pt
                            : leftBoundary.vertices_) {
                        leftPoints.append(pointMapToView(pt)
                        );
                        allPoints.append(pointMapToView(pt)
                        );
                    }
                    rightPoints.append(leftPoints[0]);
                    int i = 0;
                    for (
                        Eigen::Vector2d pt
                            : rightBoundary.vertices_) {
                        rightPoints.append(pointMapToView(pt)
                        );
                        QVector <QPointF> connector;

                        connector.append(pointMapToView(pt)
                        );
                        connector.append(leftPoints[i]);
                        painter.setPen(QPen(Qt::black, 1, Qt::SolidLine, Qt::RoundCap)
                        );
                        painter.drawPolyline(connector);
                        allPoints.append(pointMapToView(pt)
                        );
                        i++;
                    }
                    rightPoints.append(leftPoints[leftPoints.size() - 1]);

                    painter.setPen(QPen(Qt::green, 3, Qt::SolidLine, Qt::RoundCap)
                    );
                    //painter.drawPolyline(rightPoints);
                    //painter.drawPolyline(leftPoints);

                }
            }

//painter.drawPoints(qtPoints);
            painter.setPen(QPen(Qt::red, 5, Qt::SolidLine, Qt::RoundCap));
            int i = 0;
            for (
                auto &point
                    : allPoints) {
                i++;
                if (i % 5 == 0) {
                }
                painter.
                        drawPoint(point);
            }
        }

        QPointF XodrViewerWindow::XodrView::pointMapToView(const Eigen::Vector2d pt) const {
            // Scales the map point by DRAW_SCALE, flips the y axis, then applies
            // mapToViewOffset_, which is computes such that it shifts the view space
            // bounding rectangle to have margins of DRAW_MARGIN on all sides.
            return QPointF(pt.x() * DRAW_SCALE + mapToViewOffset_.x(), -pt.y() * DRAW_SCALE + mapToViewOffset_.y());
        }

    }
}  // namespace aid::xodr

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

#include "bounding_rect.h"
#include "xodr/xodr_map.h"

namespace aid {
    namespace xodr {

        static constexpr float DRAW_SCALE = 8 * 4;
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


        constexpr double padding = 10;
        constexpr double driving_elevation = 0.1;
        constexpr double sidewalk_elevation = 0.3;
        constexpr double border_elevation = 0.35;
        constexpr int noiseScale = 5;
        constexpr int noiseHeight = 5;
        constexpr int numPoints = 254;

        double getHeight(double x, double y, double minY, double minX, double width) {
            static siv::PerlinNoise noise(32);
            return noise.noise((x - minX) / width * noiseScale, (y - minY) / width * noiseScale) * noiseHeight +
                   noiseHeight;
        }

        std::stringstream writeStreet(
                const LaneSection::BoundaryCurveTessellation &a,
                const LaneSection::BoundaryCurveTessellation &b, double minY, double minX, double width,
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

        void XodrViewerWindow::XodrView::getObjFile() {

            static int counter = 0;
            std::ofstream res;
            res.open("./out/road.obj");
            std::ofstream res2;
            res2.open("./out/terrain.obj");


            double minX = std::numeric_limits<double>::infinity();
            double maxX = std::numeric_limits<double>::lowest();
            double minY = std::numeric_limits<double>::infinity();
            double maxY = std::numeric_limits<double>::lowest();

            struct DrawLane {
                const Road *r;
                int lane_section_index;
                int left_boundary_index;
                double elevation;
            };
            std::vector<DrawLane> lanes_to_draw;

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
                        const LaneSection::BoundaryCurveTessellation &boundarya = boundaries[i];
                        const LaneSection::BoundaryCurveTessellation &boundaryb = boundaries[i + 1];

                        double elevation;
                        if (lanes[i].type() == LaneType::DRIVING) {
                            elevation = driving_elevation;
                        } else if (lanes[i].type() == LaneType::SIDEWALK) {
                            elevation = sidewalk_elevation;
                        } else if (lanes[i].type() == LaneType::BORDER) {
                            if ((i < 1 || lanes[i - 1].type() != LaneType::SIDEWALK) &&
                                (i > boundaries.size() - 1 || lanes[i + 1].type() != LaneType::SIDEWALK)) {
                                continue;
                            }
                            elevation = border_elevation;
                        } else {
                            continue;
                        }
                        DrawLane drawLane{&road, laneSectionIdx, i, elevation};
                        lanes_to_draw.push_back(std::move(drawLane));
                        for (int j = 0; j < boundarya.vertices_.size(); j++) {
                            Eigen::Vector2d ptl = boundarya.vertices_[j];
                            Eigen::Vector2d ptlr = boundaryb.vertices_[j];
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

            minX -= padding;
            maxX += padding;
            minY -= padding;
            maxY += padding;




            // roads
            int index_offset = 0;
            for (const DrawLane &drawLane : lanes_to_draw) {
                const auto &laneSections = drawLane.r->laneSections();

                const LaneSection &laneSection = laneSections[drawLane.lane_section_index];

                auto refLineTessellation = drawLane.r->referenceLine().tessellate(laneSection.startS(),
                                                                                  laneSection.endS());
                auto boundaries = laneSection.tessellateLaneBoundaryCurves(refLineTessellation);
                const auto &lanes = laneSection.lanes();

                const LaneSection::BoundaryCurveTessellation &leftBoundary = boundaries[drawLane.left_boundary_index];
                const LaneSection::BoundaryCurveTessellation &rightBoundary = boundaries[drawLane.left_boundary_index +
                                                                                         1];
                std::stringstream write =
                        writeStreet(leftBoundary,
                                    rightBoundary, minY, minX, width, index_offset, drawLane.elevation);
                res << write.rdbuf();
            }


            double delta = width / (numPoints - 1);


            res2 << "o terrain" << std::endl;

            for (int c = 0; c < numPoints; c++) {
                for (int r = 0; r < numPoints; r++) {
                    int x = minX + c * delta;
                    int y = minY + r * delta;
                    res2 << "v " << x << " " << y << " " << getHeight(x, y, minX, minY, width) << std::endl;
                }
            }

            for (int c = 0; c < numPoints - 1; c++) {
                for (int r = 0; r < numPoints - 1; r++) {
                    int startNum = 1 + c + r * numPoints;
                    res2 << "f " << startNum << " " << startNum + numPoints + 1 << " "
                         << startNum + numPoints << std::endl;
                    res2 << "f " << startNum << " " << startNum + 1 << " "
                         << startNum + numPoints + 1 << std::endl;
                }
            }
            res.close();
            res2.close();
            std::cout << "Finished writing file." << std::endl << std::flush;
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

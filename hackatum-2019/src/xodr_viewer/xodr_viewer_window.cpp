#include "xodr_viewer_window.h"

#include <QtGui/QPainter>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QListWidgetItem>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QScrollArea>
#include <iosfwd>

#include "bounding_rect.h"
#include "xodr/xodr_map.h"

namespace aid {
    namespace xodr {

        static constexpr float DRAW_SCALE = 8 * 2;
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

            void setMap(std::unique_ptr <XodrMap> &&xodrMap);

            virtual void paintEvent(QPaintEvent *evnt) override;

            virtual std::stringstream getObjFile();

        private:
            /**
             * @brief Converts a point form XODR map coordinates to view coordinates.
             *
             * @param pt            The point in map coordinates.
             * @return              The point in view coordinates.
             */
            QPointF pointMapToView(const Eigen::Vector2d pt) const;

            std::unique_ptr <XodrMap> xodrMap_;

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
                std::unique_ptr <XodrMap> xodrMap(new XodrMap(std::move(fromFileRes.value())));
                xodrView_->setMap(std::move(xodrMap));
            } else {
                std::cout << "Errors: " << std::endl;
                for (const auto &err : fromFileRes.errors()) {
                    std::cout << err.description() << std::endl;
                }

                QMessageBox::critical(this, "XODR Viewer", QString("Failed to load xodr file %1.").arg(path));
            }
        }

        void XodrViewerWindow::XodrView::setMap(std::unique_ptr <XodrMap> &&xodrMap) {
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


        std::stringstream XodrViewerWindow::XodrView::getObjFile() {
            std::stringstream res;
            int seg_num = 0;
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

                    // lanes
                    // Find first and last lane
                    int left = -1;
                    int right = -1;
                    for (size_t i = 0; i < boundaries.size(); i++) {
                        const LaneSection::BoundaryCurveTessellation &boundary = boundaries[i];
                        if (lanes[i].type() == LaneType::BORDER || lanes[i].type() == LaneType::DRIVING) {
                            if (left == -1) {
                                left = i;
                            }
                            right = i;
                        }
                    }
                    if (left == -1 || right == -1) continue;
                    const LaneSection::BoundaryCurveTessellation &leftBoundary = boundaries[left];
                    const LaneSection::BoundaryCurveTessellation &rightBoundary = boundaries[right];
                    int size = leftBoundary.vertices_.size();
                    res << "o segment_num_" << seg_num++ << std::endl;
                    for (int j = 0; j < size; j++) {
                        Eigen::Vector2d ptl = leftBoundary.vertices_[j];
                        Eigen::Vector2d ptlr = rightBoundary.vertices_[j];
                        res << "v " << ptl.x() << " " << ptl.y() << " 1" << std::endl;
                        res << "v " << ptlr.x() << " " << ptlr.y() << " 1" << std::endl;
                    }
                    for (int j = 0; j < size; j++) {
                        Eigen::Vector2d ptl = leftBoundary.vertices_[j];
                        Eigen::Vector2d ptlr = rightBoundary.vertices_[j];
                        res << "v " << ptl.x() << " " << ptl.y() << " 0" << std::endl;
                        res << "v " << ptlr.x() << " " << ptlr.y() << " 0" << std::endl;
                    }
                    // triangles = road surface
                    for (int j = 1; j <= size -  2; j++) {
                        res << "f " << j << " " << j + 1 << " " << j + 2 << std::endl;
                    }
                    // sides
                    for (int j = 1; j <= size -  1; j++) {
                        res << "f " << j << " " << j + size*2 << " " << j + 2 << " " << j + 2 + size*2 << std::endl;
                    }
                    res << "f " << 1 << " " << 1 + size*2 << " " << 2 << " " << 2 + size*2 << std::endl;
                    res << "f " << size*2 << " " << size*2 + size*2 << " " << size*2 - 1 << " " << size*2 + size*2 - 1 << std::endl;
                }
            }
            return res;
        }

        void XodrViewerWindow::XodrView::paintEvent(QPaintEvent *) {
            QPainter painter(this);

            std::cout << getObjFile().rdbuf();

            QVector <QPointF> allPoints;



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

                    // lanes
                    // Find first and last lane
                    int left = -1;
                    int right = -1;
                    for (size_t i = 0; i < boundaries.size(); i++) {
                        const LaneSection::BoundaryCurveTessellation &boundary = boundaries[i];
                        if (lanes[i].type() == LaneType::BORDER || lanes[i].type() == LaneType::DRIVING) {
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
                    for (Eigen::Vector2d pt : leftBoundary.vertices_) {
                        leftPoints.append(pointMapToView(pt));
                        allPoints.append(pointMapToView(pt));
                    }
                    rightPoints.append(leftPoints[0]);
                    int i = 0;
                    for (Eigen::Vector2d pt : rightBoundary.vertices_) {
                        rightPoints.append(pointMapToView(pt));
                        QVector <QPointF> connector;

                        connector.append(pointMapToView(pt));
                        connector.append(leftPoints[i]);
                        painter.setPen(QPen(Qt::black, 1, Qt::SolidLine, Qt::RoundCap));
                        painter.drawPolyline(connector);
                        allPoints.append(pointMapToView(pt));
                        i++;
                    }
                    rightPoints.append(leftPoints[leftPoints.size()-1]);

                    painter.setPen(QPen(Qt::green, 3, Qt::SolidLine, Qt::RoundCap));
                    painter.drawPolyline(rightPoints);
                    painter.drawPolyline(leftPoints);
//
//
//                    for (size_t i = 0; i < boundaries.size(); i++) {
//                        if (i != begin && i != end) continue;
//                        const LaneSection::BoundaryCurveTessellation &boundary = boundaries[i];
//                        if (i == 0) {
//                            // The left-most boundary. Only render it if the left-most
//                            // lane is visible.
//                            if (!showLaneType(lanes[i].type())) {
//                                continue;
//                            }
//
//                        } else if (i == boundaries.size() - 1) {
//                            // The right-most boundary. Only render it if the right-most
//                            // lane is visible.
//                            if (!showLaneType(lanes[i - 1].type())) {
//                                continue;
//                            }
//
//
//                        } else {
//                            // A boundary between two lanes. Render it if at least one
//                            // of the two adjacent lanes is visible.
//                            if (!showLaneType(lanes[i - 1].type()) &&
//                                !showLaneType(lanes[i].type())) {
//                                continue;
//                            }
//                        }
//
//                        QVector <QPointF> qtPoints;
//                        for (Eigen::Vector2d pt : boundary.vertices_) {
//                            qtPoints.append(pointMapToView(pt));
//                            allPoints.append(pointMapToView(pt));
//                        }
//                        painter.drawPolyline(qtPoints);
//                    }
                }
            }

            //painter.drawPoints(qtPoints);
            painter.setPen(QPen(Qt::red, 5, Qt::SolidLine, Qt::RoundCap));
            int i = 0;
            for (auto &point : allPoints) {
                i++;
                if (i % 5 == 0) {}
                painter.drawPoint(point);
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
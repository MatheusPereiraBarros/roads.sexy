//
// Created by workstation on 11/23/19.
//

#include <bits/unique_ptr.h>
#include "xodr_converter.h"




#include "bounding_rect.h"
#include "xodr/xodr_map.h"


void XodrViewerWindow::setMap(std::unique_ptr <XodrMap> &&xodrMap) {
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



std::stringstream writeOBJFile() {
    QVector <QPointF> allPoints;

    std::vector<std::vector<std::pair<std::pair<double>>>> l;

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
                connector.
                        connector.append(pointMapToView(pt));
                connector.append(leftPoints[i]);
                painter.setPen(QPen(Qt::black, 1, Qt::SolidLine, Qt::RoundCap));
                painter.drawPolyline(connector);
                allPoints.append(pointMapToView(pt));
                i++;
            }
            rightPoints.append(leftPoints[leftPoints.size() - 1]);

            painter.setPen(QPen(Qt::green, 3, Qt::SolidLine, Qt::RoundCap));
            painter.drawPolyline(rightPoints);
            painter.drawPolyline(leftPoints);
        }
    }
}
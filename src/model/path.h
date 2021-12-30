#pragma once

#include <geometry/polyline.h>

#include <common/aggregable.h>

#include <model/renderable.h>
#include <model/pathsettings.h>
#include <model/offsettedpath.h>

#include <serializer/access.h>

#include <QTransform>

namespace model
{

class Layer;

class Path : public Renderable, public Common::Aggregable<Path>
{
	Q_OBJECT;

	friend serializer::Access<Path>;

private:
	geometry::Polyline m_basePolyline;
	std::unique_ptr<model::OffsettedPath> m_offsettedPath;
	PathSettings m_settings;
	Layer *m_layer;
	bool m_globallyVisible;

	void updateGlobalVisibility();

public:
	explicit Path(geometry::Polyline &&basePolyline, const std::string &name, const PathSettings& settings);
	explicit Path() = default;

	static ListUPtr FromPolylines(geometry::Polyline::List &&polylines, const PathSettings &settings, const std::string &layerName);

	Layer &layer();
	const Layer &layer() const;
	void setLayer(Layer &layer);

	const geometry::Polyline &basePolyline() const;
	geometry::Polyline::List finalPolylines() const;

	model::OffsettedPath *offsettedPath() const;
	void offset(float margin, float minimumPolylineLength, float minimumArcLength);
	void resetOffset();

	void transform(const Eigen::Affine2d &matrix);

	bool isPoint() const;

	const PathSettings &settings() const;
	PathSettings &settings();

	geometry::CuttingDirection cuttingDirection() const;

	bool globallyVisible() const;

Q_SIGNALS:
	void globalVisibilityChanged(bool globallyVisible);
	void offsettedPathChanged();
	void basePolylineTransformed();
};

}

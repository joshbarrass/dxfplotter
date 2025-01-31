#include <polyline.h>
#include <utils.h>
#include <cavcutils.h>

namespace geometry
{

Polyline::Polyline(const cavc::Polyline<double> &polyline)
{
	m_bulges.resize(polyline.isClosed() ? polyline.size() : polyline.size() - 1);

	polyline.visitSegIndices([&polyline, this](size_t i, size_t j){
		m_bulges[i] = Bulge(polyline[i], polyline[j]);
		return true;
	});
}

cavc::Polyline<double> Polyline::toCavc() const
{
	cavc::Polyline<double> ccPolyline;

	// Convert to CAVC polyline
	forEachBulge([&ccPolyline](const Bulge &bulge) {
		const QVector2D &start = bulge.start();
		ccPolyline.addVertex(start.x(), start.y(), bulge.tangent());
	});

	const bool closed = isClosed();
	if (!closed) {
		const QVector2D &endV = end();
		ccPolyline.addVertex(endV.x(), endV.y(), 0.0f);
	}

	ccPolyline.isClosed() = closed;

	return ccPolyline;
}

cavc::Polyline<double> Polyline::toCavc(Orientation expectedOrientation) const
{
	if (orientation() != expectedOrientation) {
		return inverse().toCavc();
	}
	return toCavc();
}

Polyline::Polyline(Bulge::List &&bulges)
	:m_bulges(bulges)
{
	assert(!m_bulges.empty());
}

const QVector2D &Polyline::start() const
{
	assert(!m_bulges.empty());

	return m_bulges.front().start();
}

QVector2D &Polyline::start()
{
	assert(!m_bulges.empty());

	return m_bulges.front().start();
}

const QVector2D &Polyline::end() const
{
	assert(!m_bulges.empty());

	return m_bulges.back().end();
}

QVector2D &Polyline::end()
{
	assert(!m_bulges.empty());

	return m_bulges.back().end();
}

bool Polyline::isClosed() const
{
	assert(!m_bulges.empty());

	return (start() == end());
}

bool Polyline::isPoint() const
{
	assert(!m_bulges.empty());

	return isClosed() && (m_bulges.size() == 1);
}

bool Polyline::isLine() const
{
	assert(!m_bulges.empty());

	return (m_bulges.size() == 1) && m_bulges.front().isLine();
}

float Polyline::length() const
{
	assert(!m_bulges.empty());

	return std::accumulate(m_bulges.begin(), m_bulges.end(), 0.0f,
		[](float sum, const Bulge &bulge){ return sum + bulge.length(); });
}

inline float winding(const QVector2D &start, const QVector2D &end)
{
	return (end.x() - start.x()) * (end.y() + start.y());
}

inline float winding(const Bulge &bulge)
{
	return winding(bulge.start(), bulge.end());
}

Orientation Polyline::orientation() const
{
	assert(!m_bulges.empty() && isClosed());

	const float windingSum = std::accumulate(m_bulges.begin(), m_bulges.end(), 0.0f,
		[](float sum, const Bulge &bulge){ return sum + winding(bulge); });

	return (windingSum > 0) ? Orientation::CW : Orientation::CCW;
}

Polyline &Polyline::invert()
{
	for (Bulge &bulge : m_bulges) {
		bulge.invert();
	}

	std::reverse(m_bulges.begin(), m_bulges.end());

	return *this;
}

Polyline Polyline::inverse() const
{
	Polyline inversed = *this;
	return inversed.invert();
}

Polyline& Polyline::operator+=(const Polyline &other)
{
	m_bulges.insert(m_bulges.end(), other.m_bulges.begin(), other.m_bulges.end());

	return *this;
}

Polyline::List Polyline::offsetted(float margin) const
{
	if (isPoint()) {
		return {*this};
	}

	// Offset CAVC polyline
	std::vector<cavc::Polyline<double> > offsettedCcPolylines = cavc::parallelOffset(toCavc(), (double)margin);

	// Convert back to polylines
	Polyline::List offsettedPolylines(offsettedCcPolylines.size());
	std::transform(offsettedCcPolylines.begin(), offsettedCcPolylines.end(), offsettedPolylines.begin(),
		[](const cavc::Polyline<double> &polyline) {
			return Polyline(polyline);
		});

	return offsettedPolylines;
}

void Polyline::transform(const QTransform &matrix)
{
	transformBulge([&matrix](Bulge &bulge){
		bulge.transform(matrix);
	});
}

bool Polyline::operator==(const Polyline &other) const
{
	return m_bulges == other.m_bulges;
}

}

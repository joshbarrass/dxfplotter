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

Polyline::Polyline(Bulge::List &&bulges)
	:m_bulges(bulges)
{
	assert(!m_bulges.empty());
}

const Eigen::Vector2d &Polyline::start() const
{
	assert(!m_bulges.empty());

	return m_bulges.front().start();
}

Eigen::Vector2d &Polyline::start()
{
	assert(!m_bulges.empty());

	return m_bulges.front().start();
}

const Eigen::Vector2d &Polyline::end() const
{
	assert(!m_bulges.empty());

	return m_bulges.back().end();
}

Eigen::Vector2d &Polyline::end()
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

double Polyline::length() const
{
	assert(!m_bulges.empty());

	return std::accumulate(m_bulges.begin(), m_bulges.end(), 0.0f,
		[](double sum, const Bulge &bulge2){ return sum + bulge2.length(); });
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

Polyline::List Polyline::offsetted(double margin) const
{
	if (isPoint()) {
		return {*this};
	}

	cavc::Polyline<double> ccPolyline;

	// Convert to CAVC polyline
	forEachBulge([&ccPolyline](const Bulge &bulge) {
		const Eigen::Vector2d &start = bulge.start();
		ccPolyline.addVertex(start.x(), start.y(), bulge.tangent());
	});

	const bool closed = isClosed();
	if (!closed) {
		const Eigen::Vector2d &endV = end();
		ccPolyline.addVertex(endV.x(), endV.y(), 0.0f);
	}

	ccPolyline.isClosed() = closed;
	// Offset CAVC polyline
	std::vector<cavc::Polyline<double> > offsettedCcPolylines = cavc::parallelOffset(ccPolyline, (double)margin);

	// Convert back to polylines
	Polyline::List offsettedPolylines(offsettedCcPolylines.size());
	std::transform(offsettedCcPolylines.begin(), offsettedCcPolylines.end(), offsettedPolylines.begin(),
		[](const cavc::Polyline<double> &polyline) {
			return Polyline(polyline);
		});

	return offsettedPolylines;
}

void Polyline::transform(const Eigen::Affine2d &matrix)
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

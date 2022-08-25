#include <task.h>

#include <iterator>

namespace model
{

void Task::initPathsFromLayers()
{
	for (const Layer::UPtr &layer : m_layers) {
		layer->forEachChild([this](Path &path){ m_paths.push_back(&path); });
	}

	// Register selection/deselection on all paths.
	forEachPath([this](Path &path) {
		connect(&path, &Path::selectedChanged, this, [this, &path](bool selected){
			const auto &pathIt = std::find(m_selectedPaths.begin(), m_selectedPaths.end(), &path);
			const bool exists = (pathIt != m_selectedPaths.end());

			if (selected && !exists) {
				m_selectedPaths.push_back(&path);
			}
			else if (exists) {
				m_selectedPaths.erase(pathIt);
			}

			emit pathSelectedChanged(path, selected);
			emit selectionChanged(m_selectedPaths.size());
		});
	});
}

void Task::initStackFromSortedPaths()
{
	struct PathLength
	{
		Path *path;
		float length;

		PathLength() = default;

		explicit PathLength(Path *path)
			:path(path),
			length(path->basePolyline().length())
		{
		}

		bool operator<(const PathLength& other) const
		{
			return length < other.length;
		}
	};

	m_stack.resize(m_paths.size());

	std::vector<PathLength> pathLengths(m_paths.size());
	std::transform(m_paths.begin(), m_paths.end(), pathLengths.begin(),
		[](Path *path){ return PathLength(path); });

	std::sort(pathLengths.begin(), pathLengths.end());

	std::transform(pathLengths.begin(), pathLengths.end(),
		m_stack.begin(), [](const PathLength &pathLength){ return pathLength.path; });
}

Task::Task(Layer::ListUPtr &&layers)
	:m_layers(std::move(layers))
{
	initPathsFromLayers();
	//initStackFromSortedPaths();
}

int Task::pathCount() const
{
	return m_paths.size();
}

const Path &Task::pathAt(int index) const
{
	assert(0 <= index && index < pathCount());
	return *m_stack[index];
}

Path &Task::pathAt(int index)
{
	assert(0 <= index && index < pathCount());
	return *m_stack[index];
}

int Task::pathIndexFor(const Path &path) const
{
	const Path::ListPtr::const_iterator it = std::find(m_stack.cbegin(), m_stack.cend(), &path);

	assert(it != m_stack.cend());

	return std::distance(m_stack.cbegin(), it);
}

void Task::movePath(int index, MoveDirection direction)
{
	assert(0 <= index && index < pathCount());

	const int newIndex = index + direction;

	if (0 <= newIndex && newIndex < pathCount()) {
		std::swap(m_stack[index], m_stack[newIndex]);
	}
}

void Task::resetCutterCompensationSelection()
{
	forEachSelectedPath([](model::Path &path){ path.resetOffset(); });
}

void Task::cutterCompensationSelection(float scaledRadius, float minimumPolylineLength, float minimumArcLength)
{
	forEachSelectedPath([scaledRadius, minimumPolylineLength, minimumArcLength](Path &path){
		path.offset(scaledRadius, minimumPolylineLength, minimumArcLength);
	});
}

void Task::pocketSelection(float radius, float minimumPolylineLength, float minimumArcLength)
{
	if (m_selectedPaths.empty()) {
		return;
	}

	Path *border = m_selectedPaths.front();
	const Path::ListCPtr islands(m_selectedPaths.begin() + 1, m_selectedPaths.end());
	border->pocket(islands, radius, minimumPolylineLength, minimumArcLength);
}

void Task::transformSelection(const QTransform& matrix)
{
	forEachSelectedPath([&matrix](Path &path){ path.transform(matrix); });
}

void Task::hideSelection()
{
	forEachSelectedPath([](Path &path){
		path.setVisible(false);
	});
}

void Task::showHidden()
{
	forEachPath([](Path &path){
		if (!path.visible()) {
			path.setVisible(true);
			path.setSelected(true);
		}
	});
}

int Task::layerCount() const
{
	return m_layers.size();
}

const Layer &Task::layerAt(int index) const
{
	assert(0 <= index && index < layerCount());
	return *m_layers[index];
}

Layer &Task::layerAt(int index)
{
	assert(0 <= index && index < layerCount());
	return *m_layers[index];
}

int Task::layerIndexFor(const Layer &layer) const
{
	const Layer::ListUPtr::const_iterator it = std::find_if(m_layers.cbegin(), m_layers.cend(),
			[&layer](const Layer::UPtr &ptr) { return ptr.get() == &layer; });

	assert(it != m_layers.cend());

	return std::distance(m_layers.cbegin(), it);
}

std::pair<int, int> Task::layerAndPathIndexFor(const Path &path) const
{
	for (int layerIndex = 0, size = m_layers.size(); layerIndex < size; ++layerIndex) {
		const Layer &layer = *m_layers[layerIndex];
		const int childIndex = layer.childIndexFor(path);
		if (childIndex != -1) {
			return std::make_pair(layerIndex, childIndex);
		}
	}

	assert(false && "layer not found");

	return std::make_pair(-1, -1);
}

}

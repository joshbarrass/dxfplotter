#pragma once

#include <model/path.h>
#include <model/layer.h>

#include <serializer/access.h>

namespace model
{

class Task : public QObject, public common::Aggregable<Task>
{
	Q_OBJECT;

	friend serializer::Access<Task>;

private:
	Path::ListPtr m_paths;
	Layer::ListUPtr m_layers;

	Path::ListPtr m_stack;
	Path::ListPtr m_selectedPaths;

	void initPathsFromLayers();
	void initStackFromSortedPaths();

public:
	enum class MoveDirection
	{
		UP = -1,
		DOWN = 1
	};

	explicit Task() = default;
	explicit Task(Layer::ListUPtr &&layers);

	int pathCount() const;
	const Path &pathAt(int index) const;
	Path &pathAt(int index);
	int pathIndexFor(const Path &path) const;

	void movePath(int index, MoveDirection direction);

	template <class Functor>
	void forEachPathInStack(Functor &&functor) const
	{
		for (const Path *path : m_stack) {
			functor(*path);
		}
	}
	
	template <class Functor>
	void forEachPath(Functor &&functor)
	{
		for (Path *path : m_paths) {
			functor(*path);
		}
	}

	template <class Functor>
	void forEachPath(Functor &&functor) const
	{
		for (const Path *path : m_paths) {
			functor(static_cast<const Path &>(*path));
		}
	}

	template <class Functor>
	void forEachSelectedPath(Functor &&functor) const
	{
		const Path::ListPtr selectedPaths(m_selectedPaths);
		for (Path *path : selectedPaths) {
			functor(*path);
		}
	}

	void resetCutterCompensationSelection();
	void cutterCompensationSelection(float scaledRadius, float minimumPolylineLength, float minimumArcLength);
	void pocketSelection(float radius, float minimumPolylineLength, float minimumArcLength);
	void transformSelection(const QTransform& matrix);
	void hideSelection();
	void showHidden();

	int layerCount() const;
	const Layer &layerAt(int index) const;
	Layer &layerAt(int index);
	int layerIndexFor(const Layer &layer) const;
	std::pair<int, int> layerAndPathIndexFor(const Path &path) const;

Q_SIGNALS:
	void pathSelectedChanged(Path &path, bool selected);
	void selectionChanged(int size);
};

template <typename T>
inline T operator+(const T &a, const Task::MoveDirection& direction)
{
	return a + static_cast<int>(direction);
}

}


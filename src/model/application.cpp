#include <application.h>
#include <geometry/assembler.h>
#include <geometry/cleaner.h>

#include <importer/dxf/importer.h>
#include <importer/dxfplot/importer.h>

#include <exporter/gcode/exporter.h>
#include <exporter/dxfplot/exporter.h>

#include <common/exception.h>

#include <QMimeDatabase>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QDebug>

namespace model
{

static const QString configFileName = "config.yml";

/** Retrieves application config file path
 * @return config file path
 */
static std::string configFilePath()
{
	const QDir dir = QDir(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation));

	// Ensure the path exists
	dir.mkpath(".");

	const QString path = dir.filePath(configFileName);

	return path.toStdString();
}

QString Application::baseName(const QString& fileName)
{
	const QFileInfo fileInfo(fileName);
	return fileInfo.absoluteDir().filePath(fileInfo.baseName());
}

void Application::resetLastSavedFileNames()
{
	m_lastSavedDxfplotFileName.clear();
	m_lastSavedGcodeFileName.clear();
}

PathSettings Application::defaultPathSettings() const
{
	const config::Profiles::Profile::DefaultPath &defaultPath = m_defaultProfileConfig->defaultPath();
	return PathSettings(defaultPath.planeFeedRate(), defaultPath.depthFeedRate(), defaultPath.intensity(), defaultPath.depth());
}

const config::Tools::Tool *Application::findTool(const std::string &name) const
{
	const config::Tools &tools = m_config.root().tools();
	if (tools.has(name)) {
		return &tools[name];
	}

	return nullptr;
}

const config::Profiles::Profile *Application::findProfile(const std::string &name) const
{
	const config::Profiles &profiles = m_config.root().profiles();
	if (profiles.has(name)) {
		return &profiles[name];
	}

	return nullptr;
}

void Application::cutterCompensation(float scale)
{
	const config::Import::Dxf &dxf = m_config.root().import().dxf();

	const float radius = m_openedDocument->toolConfig().general().radius();
	const float scaledRadius = radius * scale;

	m_openedDocument->task().forEachSelectedPath([scaledRadius, minimumPolylineLength=(float)dxf.minimumPolylineLength(),
		minimumArcLength=(float)dxf.minimumArcLength()](model::Path &path){
			path.offset(scaledRadius, minimumPolylineLength, minimumArcLength);
	});
}

Task::UPtr Application::createTaskFromDxfImporter(const importer::dxf::Importer& importer)
{
	const config::Import::Dxf &dxf = m_config.root().import().dxf();

	Layer::ListUPtr layers;
	for (importer::dxf::Layer &importerLayer : importer.layers()) {
		// Merge polylines to create longest contours
		geometry::Assembler assembler(importerLayer.polylines(), dxf.assembleTolerance());
		// Remove small bulges
		geometry::Cleaner cleaner(assembler.polylines(), dxf.minimumPolylineLength(), dxf.minimumArcLength());

		const std::string &layerName = importerLayer.name();

		// Create paths from merged and cleaned polylines of one layer
		Path::ListUPtr children = Path::FromPolylines(cleaner.polylines(), defaultPathSettings(), layerName);

		Layer::UPtr& layer = layers.emplace_back(std::make_unique<Layer>(layerName, std::move(children)));
	}

	return std::make_unique<Task>(std::move(layers));
}

Application::Application()
	:m_config(configFilePath()),
	// Default select first tool
	m_defaultToolConfig(&m_config.root().tools().first()),
	// Default select first profile
	m_defaultProfileConfig(&m_config.root().profiles().first())
{
}

config::Config &Application::config()
{
	return m_config;
}

void Application::setConfig(config::Config &&config)
{
	m_config = std::move(config);
	emit configChanged(m_config);
}

bool Application::selectTool(const QString &toolName)
{
	const std::string name = toolName.toStdString();
	const config::Tools::Tool *tool = findTool(name);

	if (tool) {
		if (m_openedDocument) {
			m_openedDocument->setToolConfig(*tool);
		}
		m_defaultToolConfig = tool;

		return true;
	}

	return false;
}

void Application::defaultToolFromCmd(const QString &toolName)
{
	if (!selectTool(toolName)) {
		qCritical() << "Invalid tool name " << toolName;
	}
}

bool Application::selectProfile(const QString &profileName)
{
	const std::string name = profileName.toStdString();
	const config::Profiles::Profile *profile = findProfile(name);

	if (profile) {
		if (m_openedDocument) {
			m_openedDocument->setProfileConfig(*profile);
		}
		m_defaultProfileConfig = profile;

		return true;
	}

	return false;
}

void Application::defaultProfileFromCmd(const QString &profileName)
{
	if (!selectProfile(profileName)) {
		qCritical() << "Invalid profile name " << profileName;
	}
}

const QString &Application::lastHandledFileBaseName() const
{
	return m_lastHandledFileBaseName;
}

const QString &Application::lastSavedDxfplotFileName() const
{
	return m_lastSavedDxfplotFileName;
}

const QString &Application::lastSavedGcodeFileName() const
{
	return m_lastSavedGcodeFileName;
}

void Application::loadFileFromCmd(const QString &fileName)
{
	if (!fileName.isEmpty()) {
		loadFile(fileName);
	}
}

bool Application::loadFile(const QString &fileName)
{
	qInfo() << "Opening " << fileName;

	const QMimeDatabase db;
	const QMimeType mime = db.mimeTypeForFile(fileName);
	const QString mineName = mime.name();

	if (mineName == "image/vnd.dxf") {
		if (!loadFromDxf(fileName)) {
			return false;
		}
	}
	else if (mineName == "text/plain") {
		loadFromDxfplot(fileName);
	}
	else {
		qCritical() << "Invalid file type: " << fileName;
		return false;
	}

	m_lastHandledFileBaseName = baseName(fileName);
	resetLastSavedFileNames();

	// Update window title based on file name.
	const QFileInfo fileInfo(fileName);
	const QString title = fileInfo.fileName();
	emit titleChanged(title);

	return true;
}

bool Application::loadFromDxf(const QString &fileName)
{
	const config::Import::Dxf &dxf = m_config.root().import().dxf();

	try {
		importer::dxf::Importer importer(fileName.toStdString(), dxf.splineToArcPrecision(), dxf.minimumSplineLength());
 
		m_openedDocument = std::make_unique<Document>(createTaskFromDxfImporter(importer), *m_defaultToolConfig, *m_defaultProfileConfig);
	}
	catch (const Common::FileCouldNotOpenException&) {
		qCritical() << "File not found:" << fileName;
		return false;
	}

	emit documentChanged(m_openedDocument.get());

	return true;
}

bool Application::loadFromDxfplot(const QString &fileName)
{
	try {
		importer::dxfplot::Importer importer(m_config.root().tools(), m_config.root().profiles());
 
		m_openedDocument = importer(fileName.toStdString());
	}
	catch (const Common::FileCouldNotOpenException&) {
		return false;
	}

	emit documentChanged(m_openedDocument.get());

	return true;
}

bool Application::saveToGcode(const QString &fileName)
{
	try {
		exporter::gcode::Exporter exporter(m_openedDocument->toolConfig(), m_openedDocument->profileConfig().gcode(), exporter::gcode::Exporter::ExportConfig);
		const bool saved = saveToFile(exporter, fileName);
		if (saved) {
			m_lastSavedGcodeFileName = fileName;
		}
		return saved;
	}
	catch (const std::exception &exception) {
		emit errorRaised(exception.what());
	}

	return false;
}

bool Application::saveToDxfplot(const QString &fileName)
{
	exporter::dxfplot::Exporter exporter;

	const bool saved = saveToFile(exporter, fileName);
	if (saved) {
		m_lastSavedDxfplotFileName = fileName;
	}

	return saved;
}

void Application::leftCutterCompensation()
{
	cutterCompensation(1.0f);
}

void Application::rightCutterCompensation()
{
	cutterCompensation(-1.0f);
}

void Application::resetCutterCompensation()
{
	Task &task = m_openedDocument->task();
	task.forEachSelectedPath([](model::Path &path){ path.resetOffset(); });
}

void Application::transformSelection(const Eigen::Affine2d& matrix)
{
	Task &task = m_openedDocument->task();
	task.forEachSelectedPath([&matrix](model::Path &path){ path.transform(matrix); });
}

void Application::hideSelection()
{
	Task &task = m_openedDocument->task();
	task.forEachSelectedPath([](model::Path &path){
		path.setVisible(false);
	});
}

void Application::showHidden()
{
	Task &task = m_openedDocument->task();
	task.forEachPath([](model::Path &path){
		if (!path.visible()) {
			path.setVisible(true);
			path.setSelected(true);
		}
	});
}

}

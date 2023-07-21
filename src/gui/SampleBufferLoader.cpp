#include "SampleBufferLoader.h"

#include <QMessageBox>

namespace lmms::gui {
std::unique_ptr<SampleBuffer> SampleBufferLoader::loadFromFile(const QString& filePath)
{
	try
	{
        return std::make_unique<SampleBuffer>(filePath);
	}
	catch (const std::runtime_error& error)
	{
        displayError(QString::fromStdString(error.what()));
        return nullptr;
	}
}

std::unique_ptr<SampleBuffer> SampleBufferLoader::loadFromBase64(const QString& base64, int sampleRate)
{
    try
	{
        return std::make_unique<SampleBuffer>(base64.toUtf8().toBase64(), sampleRate);
	}
	catch (const std::runtime_error& error)
	{
        displayError(QString::fromStdString(error.what()));
        return nullptr;
	}
}

void SampleBufferLoader::displayError(const QString& message)
{
	QMessageBox::critical(nullptr, QObject::tr("Error loading sample"), message);
}
} // namespace lmms::gui
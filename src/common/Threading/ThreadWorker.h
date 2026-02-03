#pragma once

#include <QObject>
#include <QThread>
#include <functional>

#include "Logger.h"
#include "LoggerDefines.h"

class ThreadWorker final : public QObject
{
	Q_OBJECT

public:
	using TaskFunction = std::function<void()>;

	explicit ThreadWorker(TaskFunction task, QObject* parent = nullptr) : QObject(parent), taskFunction(std::move(task)) { }

signals:
	void finished();  // Signal when the task is finished

public slots:
	void runTask()
	{
        try
        {
            if (taskFunction)
                taskFunction();  // Execute task
        }
        catch (const std::exception& ex)
        {
            LOG_ERROR(std::string("ThreadWorker task failed: ") + ex.what());
        }
        catch (...)
        {
            LOG_ERROR("ThreadWorker task failed: unknown exception");
        }


		emit finished();
	}

private:
	TaskFunction taskFunction;  // Any function
};

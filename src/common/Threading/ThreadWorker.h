#pragma once

#include <QObject>
#include <QThread>
#include <functional>

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
		if (taskFunction)
			taskFunction();  // Execute task

		emit finished();
	}

private:
	TaskFunction taskFunction;  // Any function
};


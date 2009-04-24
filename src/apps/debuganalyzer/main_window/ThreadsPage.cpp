/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "ThreadsPage.h"

#include <stdio.h>

#include <new>

#include "Model.h"
#include "TableColumns.h"


// #pragma mark - ThreadsTableModel


class ThreadsPage::ThreadsTableModel : public TableModel {
public:
	ThreadsTableModel(Model* model)
		:
		fModel(model)
	{
	}

	virtual int32 CountColumns() const
	{
		return 11;
	}

	virtual int32 CountRows() const
	{
		return fModel->CountThreads();
	}

	virtual bool GetValueAt(int32 rowIndex, int32 columnIndex, Variant& value)
	{
		Model::Thread* thread = fModel->ThreadAt(rowIndex);
		if (thread == NULL)
			return false;

		switch (columnIndex) {
			case 0:
				value.SetTo(thread->ID());
				return true;
			case 1:
				value.SetTo(thread->Name(), VARIANT_DONT_COPY_DATA);
				return true;
			case 2:
				value.SetTo(thread->CreationTime());
				return true;
			case 3:
				value.SetTo(thread->DeletionTime());
				return true;
			case 4:
				value.SetTo(thread->Runs());
				return true;
			case 5:
				value.SetTo(thread->TotalRunTime());
				return true;
			case 6:
				value.SetTo(thread->Reruns());
				return true;
			case 7:
				value.SetTo(thread->TotalRerunTime());
				return true;
			case 8:
				value.SetTo(thread->Latencies());
				return true;
			case 9:
				value.SetTo(thread->TotalLatency());
				return true;
			case 10:
				value.SetTo(thread->Preemptions());
				return true;
			default:
				return false;
		}
	}

private:
	Model*	fModel;
};


// #pragma mark - ThreadsPage


ThreadsPage::ThreadsPage()
	:
	BGroupView(B_VERTICAL),
	fThreadsTable(NULL),
	fThreadsTableModel(NULL),
	fModel(NULL)

{
	SetName("Threads");

	fThreadsTable = new Table("threads list", 0);
	AddChild(fThreadsTable->ToView());

	fThreadsTable->AddColumn(new Int32TableColumn(0, "Thread", 40, 20, 1000,
		B_TRUNCATE_MIDDLE, B_ALIGN_RIGHT));
	fThreadsTable->AddColumn(new StringTableColumn(1, "Name", 80, 40, 1000,
		B_TRUNCATE_END, B_ALIGN_LEFT));
	fThreadsTable->AddColumn(new BigtimeTableColumn(2, "Creation", 80, 40, 1000,
		true, B_TRUNCATE_MIDDLE, B_ALIGN_RIGHT));
	fThreadsTable->AddColumn(new BigtimeTableColumn(3, "Deletion", 80, 40, 1000,
		false, B_TRUNCATE_MIDDLE, B_ALIGN_RIGHT));
	fThreadsTable->AddColumn(new Int64TableColumn(4, "Runs", 80, 20, 1000,
		B_TRUNCATE_END, B_ALIGN_RIGHT));
	fThreadsTable->AddColumn(new BigtimeTableColumn(5, "Run Time", 80, 20, 1000,
		false, B_TRUNCATE_END, B_ALIGN_RIGHT));
	fThreadsTable->AddColumn(new Int64TableColumn(6, "Reruns", 80, 20, 1000,
		B_TRUNCATE_END, B_ALIGN_RIGHT));
	fThreadsTable->AddColumn(new BigtimeTableColumn(7, "Rerun Time", 80, 20,
		1000, false, B_TRUNCATE_END, B_ALIGN_RIGHT));
	fThreadsTable->AddColumn(new Int64TableColumn(8, "Latencies", 80, 20, 1000,
		B_TRUNCATE_END, B_ALIGN_RIGHT));
	fThreadsTable->AddColumn(new BigtimeTableColumn(9, "Latency Time", 80, 20,
		1000, B_TRUNCATE_END, B_ALIGN_RIGHT));
	fThreadsTable->AddColumn(new Int64TableColumn(10, "Preemptions", 80, 20,
		1000, B_TRUNCATE_END, B_ALIGN_RIGHT));
}


ThreadsPage::~ThreadsPage()
{
	fThreadsTable->SetTableModel(NULL);
	delete fThreadsTableModel;
}


void
ThreadsPage::SetModel(Model* model)
{
	if (model == fModel)
		return;

	if (fModel != NULL) {
		fThreadsTable->SetTableModel(NULL);
		delete fThreadsTableModel;
		fThreadsTableModel = NULL;
	}

	fModel = model;

	if (fModel != NULL) {
		fThreadsTableModel = new(std::nothrow) ThreadsTableModel(fModel);
		fThreadsTable->SetTableModel(fThreadsTableModel);
	}
}
#include "convar.h"
#include "cdll_int.h"
#include "cdll_client_int.h"
#include "client_class.h"
#include "dt_recv.h"
#include <stdio.h>

static const char* s_szSendPropTypes[] = {
	"Int",
	"Float",
	"Vector",
	"VectorXY",
	"String",
	"Array",
	"DataTable"
};

static void DumpDataTable(FILE* fOut, RecvTable* table, int tabs = 1)
{
	fprintf(fOut, "RecvTable %s\n", table->m_pNetTableName);
	for (int i = 0; i < table->GetNumProps(); i++)
	{
		//Tabs
		for (int j = 0; j < tabs; j++)
			fputc('\t', fOut);

		RecvProp* prop = table->GetProp(i);
		fprintf(fOut, "0x%04x %s %s", prop->GetOffset(),
			s_szSendPropTypes[prop->GetType()],
			prop->GetName());

		RecvProp* array;
		switch (prop->GetType())
		{
		case DPT_Array:
			array = prop->GetArrayProp();
			fprintf(fOut, "[%d] x %d",
				prop->GetNumElements(),
				prop->GetElementStride()
				);
			break;
		case DPT_DataTable:
			DumpDataTable(fOut, prop->GetDataTable(), tabs + 1);
			break;
		}

		fprintf(fOut, "\n");
	}
}

CON_COMMAND(blackhat_dump_classes, "Dump all client classes")
{
	FILE* fOut;
	fopen_s(&fOut, "class.txt", "w");

	ClientClass* item = clientdll->GetAllClasses();
	if (item)
	{
		do {
			//fprintf(fOut, "[%d]\t%s\n", clientClass->m_pNetworkName, clientClass->m_pNetworkName);
			RecvTable* recvTable = item->m_pRecvTable;
			fprintf(fOut, "class %s [%d] ", item->GetName(), item->m_ClassID);
			if (recvTable)
			{
				DumpDataTable(fOut, recvTable, 1);
			}
			else
				fprintf(fOut, "\n");
		} while (item = item->m_pNext);
	}

	fclose(fOut);
}

CON_COMMAND(blackhat_dump_class_recvtable, "Dump class RecvTable")
{
	ClientClass* item = clientdll->GetAllClasses();
	if (item)
	{
		do {
			if (!strcmp(item->GetName(), args.Arg(1)))
				break;
		} while (item = item->m_pNext);
	}

	FILE* fOut;
	fopen_s(&fOut, "class_1.txt", "w");
	DumpDataTable(fOut, item->m_pRecvTable);
	fclose(fOut);
}
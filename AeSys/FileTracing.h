#pragma once

class EoDbLayer;

class CFileTracing : public CFile
{
public:
	CFileTracing() 
	{
	}
	virtual ~CFileTracing() 
	{
	}
	void ReadHeader(CFile& file);
	bool ReadLayer(CFile& file, EoDbLayer* layer);
	EoDbGroup* ReadGroup(CFile &file);

	void WriteHeader(CFile& file);
	void WriteLayer(CFile& file, EoDbLayer* layer);
};
// EoDb::kHeaderSection sentinel				EoUInt16 0x0101
//		{0 or more key-value pairs}
// EoDb::kEndOfSection sentinel					EoUInt16 0x01ff
// EoDb::kGroupsSection sentinel				EoUInt16 0x0104
//		Number of group definitions				EoUInt16
//		{0 or more group definitions}
// EoDb::kEndOfSection sentinel					EoUInt16 0x01ff


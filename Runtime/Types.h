#pragma once

// Types.h - ����������� ������������� ���������� � ����� � FPTL.

//
// �������� ����.
// 
// � ������� ���� ������ ������� ���� ��������� TypeInfo.
// ��� ������� ������������ ���������� ������ ��������� �����, � ������� ����� ���� �������� ������ �����������.
// ���������� ������ ����.
// ���� ��� ���������� ������������ ��� �������� ���� T, ��� T ������� � �������, �� �� ������������ � �������� ���� �������, ���������� �������������.
// ���� � � ������� �� ����������, �� ������������ ������ ������� ��������, � �� ���� ����������� � ������, ���� ������������ ����������.
//

#include "../Parser/Nodes.h"
#include <vector>
#include <algorithm>
#include <ostream>
#include <ext/hash_map>
#include <boost/bind.hpp>

namespace FPTL { namespace Runtime {

typedef std::hash_map<std::string, struct TypeInfo *> TParametersMap;

//------------------------------------------------------------------------------------------
// ���������� � ����. �� ������� ������ ������, �.�. ������ ��������� ��������� � TypeInfoRegistry.
struct TypeInfo
{
	std::string TypeName;

	std::hash_map<std::string, TypeInfo> Parameters;

	TypeInfo()
	{}
	TypeInfo(const std::string & aTypeName) : TypeName(aTypeName)
	{}
	TypeInfo(const std::string & aTypeName, const TParametersMap & aParameters) : TypeName(aTypeName)
	{
		for (auto i = aParameters.begin(); i != aParameters.end(); ++i)
		{
			Parameters.insert(std::make_pair(i->first, *i->second));
		}
	}

	// ���������� �������� ���������� ���� aTypeInfo c �������� aRef.
	static bool matchType(const TypeInfo * aTypeInfo, const TypeInfo * aRef, TParametersMap & aParametersMap);

	// ����� ������� ����������.
	friend std::ostream & operator <<(std::ostream & aStream, const TypeInfo & aTypeInfo);
};

typedef std::vector<TypeInfo> TTypeList;

//------------------------------------------------------------------------------------------

}} // FPTL::Runtime

﻿// Встроенные функции.

#ifndef STANDART_LIBRARY_H
#define STANDART_LIBRARY_H

#include "FunctionLibrary.h"

namespace FPTL {
namespace Runtime {


class StandardLibrary : public FunctionLibrary
{
public:
	StandardLibrary();
	static const std::map<std::string, TFunction> mFunctions;
};

}} // FPTL::Runtime

#endif
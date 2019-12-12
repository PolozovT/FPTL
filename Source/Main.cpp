﻿#include <limits>
#include <clocale>
#include <iostream>

#include "jemalloc/jemalloc.h"

#include "Interpreter/Interpreter.h"

void * operator new (const std::size_t count)
{
	return je_malloc(count);
}

void * operator new[](const std::size_t count)
{
	return je_malloc(count);
}

void operator delete(void * ptr) noexcept
{
	je_free(ptr);
}

void operator delete[](void * ptr) noexcept
{
	je_free(ptr);
}

int main(const int argc, const char ** argv)
{
	FPTL::Runtime::Interpreter interpreter;
	return interpreter.Eval(argc, argv);
}
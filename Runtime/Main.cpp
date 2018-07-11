﻿#include <iostream>
#include <fstream>
#include <boost/program_options.hpp>

#include "../Parser/Support.h"
#include "FSchemeGenerator.h"
#include "Run.h"
#include "IntForm/Generator.h"
#include "IntForm/InternalForm.h"
#include "IntForm/Context.h"

#include "jemalloc/jemalloc.h"

namespace po = boost::program_options;

namespace FPTL {
namespace Parser {

template <typename T> void setOption(const po::variable_value & val, std::function<void(T)> setter)
{
	if (!val.empty())
	{
		setter(val.as<T>());
	}
}

void run(const char * programPath, const int numCores, po::variables_map & vm)
{
	std::ifstream inpFile(programPath);

	if (!inpFile.good())
	{
		std::cout << "Unable to open file : " << programPath << "\n";
		return;
	}

	std::string inputStr;
	std::copy(std::istreambuf_iterator<char>(inpFile), std::istreambuf_iterator<char>(), std::back_inserter(inputStr));

	Support support;
	ASTNode * astRoot = support.getInternalForm(inputStr);
	support.getErrorList(std::cout);

	// Генерируем внутренне представление.
	if (astRoot)
	{
		std::cout << "No syntax errors found.\n";
		std::cout << "Running program: " << programPath << " on " << numCores << " work threads...\n\n";

		Runtime::FSchemeGenerator schemeGenerator;
		schemeGenerator.process(astRoot);

		const bool disableAnt = vm["disable-ant"].as<bool>() || (numCores==1);
		if (disableAnt)
			std::cout << "Anticipatory computing disabled.\n\n";

		std::unique_ptr<Runtime::FunctionalProgram> internalForm(Runtime::Generator::generate(schemeGenerator.getProgram(), disableAnt));

		Runtime::SchemeEvaluator evaluator;

		Runtime::GcConfig gcConfig;

		setOption<bool>(vm["disable-gc"], [&](bool v) {gcConfig.setEnabled(!v); });
		setOption<bool>(vm["verbose-gc"], [&](bool v) {gcConfig.setVerbose(v); });
		setOption<size_t>(vm["young-gen"], [&](size_t s) {gcConfig.setYoungGenSize(s * 1024 * 1024); });
		setOption<size_t>(vm["old-gen"], [&](size_t s) {gcConfig.setOldGenSize(s * 1024 * 1024); });
		setOption<double>(vm["old-gen-ratio"], [&](double r) {gcConfig.setOldGenThreashold(r); });

		evaluator.setGcConfig(gcConfig);

		Runtime::IFExecutionContext ctx(internalForm->main().get());
		evaluator.run(ctx, numCores, disableAnt);
	}

	delete astRoot;
}

}}

void * operator new  (std::size_t count)
{
	return ::je_malloc(count);
}

void * operator new[](std::size_t count)
{
	return ::je_malloc(count);
}

void operator delete  (void * ptr)
{
	::je_free(ptr);
}

void operator delete[](void * ptr)
{
	::je_free(ptr);
}

int main(int argc, char ** argv)
{
 	std::string programFile;
	int numCores;

	po::options_description desc("Avilable options");
	desc.add_options()
		("source-file", po::value<std::string>(&programFile)->required(), "fptl program file")
		("num-cores", po::value<int>(&numCores)->default_value(1), "number of worker threads")
		("disable-ant", po::bool_switch(), "disable anticipatory computing")
		("disable-gc", po::bool_switch(), "disable garbage collector")
		("verbose-gc", po::bool_switch(), "print garbage collector info")
		("young-gen", po::value<size_t>(), "young generation size in MiB")
		("old-gen", po::value<size_t>(), "old generation size in MiB")
		("old-gen-ratio", po::value<double>(), "old gen usage ratio to start full GC")
		;

	try
	{
		po::variables_map vm;
		po::store(po::parse_command_line(argc, argv, desc), vm);
		po::notify(vm);

		FPTL::Parser::run(programFile.c_str(), numCores, vm);
	}
	catch (std::exception & e)
	{
		std::cout
			<< e.what() << std::endl
			<< desc << std::endl;
	}

	return 0;
}
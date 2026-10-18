// lemoncheck: compiles the Sonic 3 A.I.R. base scripts plus one or more script mods
// with the same lemonscript compiler the game uses, without needing the ROM.
//
// Native engine functions are registered as no-op stubs, using the signatures from
// the game's auto-generated "_reference/cpp_core_functions.lemon".
//
// Usage: lemoncheck <base scripts main.lemon> <cpp_core_functions.lemon> <mod main.lemon>...

#include "lemon/compiler/Compiler.h"
#include "lemon/program/GlobalsLookup.h"
#include "lemon/program/Module.h"
#include "lemon/program/Program.h"
#include "lemon/program/function/FunctionWrapper.h"
#include "lemon/program/function/NativeFunction.h"
#include "lemon/program/function/ScriptFunction.h"
#include "lemon/runtime/ControlFlow.h"
#include "lemon/runtime/Runtime.h"
#include "lemon/runtime/StandardLibrary.h"

#include <fstream>
#include <iostream>
#include <set>
#include <sstream>

using namespace lemon;


class StubWrapper : public NativeFunction::FunctionWrapper
{
public:
	StubWrapper(const DataTypeDefinition* returnType, std::vector<const DataTypeDefinition*> parameterTypes) :
		mReturnType(returnType), mParameterTypes(std::move(parameterTypes)) {}

	void execute(const NativeFunction::Context context) const override
	{
		// Behave like a real native function on the value stack: take the parameters, give back a zero result
		for (size_t i = mParameterTypes.size(); i > 0; --i)
		{
			if (mParameterTypes[i - 1] == &PredefinedDataTypes::ANY)
			{
				AnyTypeWrapper value;
				value.popFromStack(context.mControlFlow);
			}
			else
			{
				context.mControlFlow.popValueStack<uint64>();
			}
		}
		if (mReturnType != &PredefinedDataTypes::VOID)
		{
			context.mControlFlow.pushValueStack<uint64>(0);
		}
	}
	const DataTypeDefinition* getReturnType() const override  { return mReturnType; }
	std::vector<const DataTypeDefinition*> getParameterTypes() const override  { return mParameterTypes; }

private:
	const DataTypeDefinition* mReturnType;
	std::vector<const DataTypeDefinition*> mParameterTypes;
};


static std::map<std::string, const DataTypeDefinition*> gTypes;

static const DataTypeDefinition* asPointer(const CustomDataType* type)  { return type; }
static const DataTypeDefinition* asPointer(const CustomDataType& type)  { return &type; }

static std::string trim(const std::string& s)
{
	const size_t a = s.find_first_not_of(" \t\r\n");
	if (a == std::string::npos)
		return "";
	const size_t b = s.find_last_not_of(" \t\r\n");
	return s.substr(a, b - a + 1);
}

static const DataTypeDefinition* lookupType(const std::string& name)
{
	auto it = gTypes.find(name);
	if (it == gTypes.end())
	{
		std::cerr << "Unknown type in reference: '" << name << "'\n";
		exit(2);
	}
	return it->second;
}

//-------------------------------------------------------//
// Simulation support                                    //
//-------------------------------------------------------//

static int64 gRegisters[16] = {};

// Memory as seen by the scripts, big-endian like the Mega Drive
class SimMemory : public MemoryAccessHandler
{
public:
	uint8  read8 (uint64 address) override  { auto it = mMemory.find(address & 0xffffffff); return (it == mMemory.end()) ? 0 : it->second; }
	uint16 read16(uint64 address) override  { return ((uint16)read8(address) << 8) | read8(address + 1); }
	uint32 read32(uint64 address) override  { return ((uint32)read16(address) << 16) | read16(address + 2); }
	uint64 read64(uint64 address) override  { return ((uint64)read32(address) << 32) | read32(address + 4); }
	void write8 (uint64 address, uint8 value) override
	{
		if (mWatchAddress != 0 && (address & 0xffffffff) == mWatchAddress)
			reportWatch(value);
		mMemory[address & 0xffffffff] = value;
	}

	uint32 mWatchAddress = 0;

	void reportWatch(uint8 value)
	{
		std::cout << "  WATCH: write 0x" << std::hex << (int)value << std::dec << " to 0x" << std::hex << mWatchAddress << " A0=0x" << (uint32)gRegisters[8] << " frame=0x" << read16(0xfffffe04) << std::dec;
		ControlFlow* controlFlow = Runtime::getActiveControlFlow();
		if (nullptr != controlFlow)
		{
			std::vector<ControlFlow::Location> locations;
			controlFlow->getCallStack(locations);
			for (const ControlFlow::Location& location : locations)
			{
				if (nullptr != location.mFunction)
					std::cout << " <- " << location.mFunction->getName().getString();
			}
		}
		std::cout << std::endl;
	}
	void write16(uint64 address, uint16 value) override  { write8(address, (uint8)(value >> 8)); write8(address + 1, (uint8)value); }
	void write32(uint64 address, uint32 value) override  { write16(address, (uint16)(value >> 16)); write16(address + 2, (uint16)value); }
	void write64(uint64 address, uint64 value) override  { write32(address, (uint32)(value >> 32)); write32(address + 4, (uint32)value); }

private:
	std::map<uint64, uint8> mMemory;
};

static SimMemory gMemory;
static std::vector<uint32> gStack;
static uint32 gButtonsPressed = 0;
static bool gLogDraws = false;

static void sim_zeroMemory(uint32 start, uint32 bytes)				{ for (uint32 i = 0; i < bytes; ++i) gMemory.write8(start + i, 0); }
static void sim_copyMemory(uint32 dest, uint32 src, uint32 bytes)	{ for (uint32 i = 0; i < bytes; ++i) gMemory.write8(dest + i, gMemory.read8(src + i)); }
static void sim_push(uint32 value)		{ gStack.push_back(value); }
static uint32 sim_pop()					{ if (gStack.empty()) { std::cout << "SIM ERROR: pop on empty stack\n"; return 0; } uint32 v = gStack.back(); gStack.pop_back(); return v; }
static uint8 sim_buttonPressed(uint8 index)					{ return (index < 32) ? ((gButtonsPressed >> index) & 1) : 0; }
static bool sim_buttonPressed2(uint8 player, uint8 index)	{ return (player == 0) && sim_buttonPressed(index); }
static void sim_setButtonsPressed(uint32 mask)	{ gButtonsPressed = mask; }
static void sim_setLogDraws(bool enable)		{ gLogDraws = enable; }
static void sim_setWatch(uint32 address)		{ gMemory.mWatchAddress = address; }
static uint32 sim_getStackSize()				{ return (uint32)gStack.size(); }
static void sim_log(StringRef label, int64 value)	{ std::cout << "  " << label.getString() << " = " << value << "\n"; }
static void sim_fail(StringRef message)			{ std::cout << "SIM FAIL: " << message.getString() << "\n"; exit(3); }
static void sim_drawSprite(uint64 key, int16 px, int16 py, uint16 atex, uint8 flags, uint16 renderQueue)
{
	if (gLogDraws)
		std::cout << "  draw sprite at " << px << ", " << py << " atex 0x" << std::hex << atex << " flags 0x" << (int)flags << " queue 0x" << renderQueue << std::dec << "\n";
}
static void sim_drawText(StringRef font, int32 px, int32 py, StringRef text, uint32 tintColor, uint8 alignment, int8 spacing, uint16 renderQueue, bool useWorldSpace)
{
	if (gLogDraws)
		std::cout << "  draw text '" << text.getString() << "' at " << px << ", " << py << " color 0x" << std::hex << tintColor << std::dec << "\n";
}
static int32 sim_getTextWidth(StringRef font, StringRef text)	{ return (int32)text.getString().length() * 8; }
static bool sim_onCharacterDied(uint8 playerIndex)				{ return true; }
static uint16 sim_getScreenWidth()	{ return 400; }
static uint16 sim_getScreenHeight()	{ return 224; }

// Names (with number of parameters) of natives that get a working implementation instead of a stub
static std::set<std::string> gImplementedNatives;

static void registerSimulationNatives(Module& module)
{
	const auto add = [&](const char* name, size_t numParams, NativeFunction::FunctionWrapper& wrapper)
	{
		module.addNativeFunction(FlyweightString(name), wrapper);
		gImplementedNatives.insert(std::string(name) + "/" + std::to_string(numParams));
	};
	add("zeroMemory", 2, wrap(&sim_zeroMemory));
	add("copyMemory", 3, wrap(&sim_copyMemory));
	add("push", 1, wrap(&sim_push));
	add("pop", 0, wrap(&sim_pop));
	add("Input.buttonPressed", 1, wrap(&sim_buttonPressed));
	add("Input.buttonPressed", 2, wrap(&sim_buttonPressed2));
	add("Renderer.drawSprite", 6, wrap(&sim_drawSprite));
	add("Renderer.drawText", 9, wrap(&sim_drawText));
	add("Renderer.getTextWidth", 2, wrap(&sim_getTextWidth));
	add("Game.onCharacterDied", 1, wrap(&sim_onCharacterDied));
	add("getScreenWidth", 0, wrap(&sim_getScreenWidth));
	add("getScreenHeight", 0, wrap(&sim_getScreenHeight));

	// Test-only functions
	module.addNativeFunction(FlyweightString("Sim.setButtonsPressed"), wrap(&sim_setButtonsPressed));
	module.addNativeFunction(FlyweightString("Sim.setLogDraws"), wrap(&sim_setLogDraws));
	module.addNativeFunction(FlyweightString("Sim.setWatch"), wrap(&sim_setWatch));
	module.addNativeFunction(FlyweightString("Sim.getStackSize"), wrap(&sim_getStackSize));
	module.addNativeFunction(FlyweightString("Sim.log"), wrap(&sim_log));
	module.addNativeFunction(FlyweightString("Sim.fail"), wrap(&sim_fail));
}

struct SimConnector : public Runtime::ExecuteConnector
{
	Runtime& mRuntime;
	std::map<uint32, const Function*>& mHooks;
	bool mStopped = false;

	SimConnector(Runtime& runtime, std::map<uint32, const Function*>& hooks) : mRuntime(runtime), mHooks(hooks) {}

	bool handleCall(const Function* func, uint64 callTarget) override
	{
		if (nullptr == func)
		{
			std::cout << "SIM ERROR: call to invalid function, target 0x" << std::hex << callTarget << std::dec << "\n";
			mStopped = true;
			return false;
		}
		return true;
	}

	bool handleReturn() override
	{
		if (mRuntime.getMainControlFlow().getCallStack().count == 0)
		{
			mStopped = true;
			return false;
		}
		return true;
	}

	bool handleExternalCall(uint64 address) override
	{
		auto it = mHooks.find((uint32)address);
		if (it == mHooks.end())
		{
			// No ROM code available, so unknown addresses just get skipped
			std::cout << "  (call to unhooked address 0x" << std::hex << address << std::dec << " skipped)\n";
			return false;
		}
		mRuntime.callFunction(*it->second);
		return true;
	}

	bool handleExternalJump(uint64 address) override
	{
		return handleExternalCall(address);
	}
};

static int runSimulation(const std::vector<const Module*>& modules, const std::string& functionName)
{
	Program program;
	for (const Module* module : modules)
		program.addModule(*module);

	// Address hooks, bound like S3AIR v26.03.28 does it ("LemonScriptProgram::collectHooksFromFunctions"):
	//  -> Each hook goes to the function that has the address-hook pragma, later modules overwrite earlier ones
	//  -> An override without the pragma does NOT get called via its address (newer engine versions changed that)
	std::map<uint32, const Function*> hooks;
	for (const ScriptFunction* function : program.getScriptFunctions())
	{
		for (const ScriptFunction::AddressHook& addressHook : function->getAddressHooks())
		{
			hooks[addressHook.mAddress] = function;
		}
	}

	const Function* func = program.getFunctionBySignature(rmx::getMurmur2_64(String(functionName.c_str())) + Function::getVoidSignatureHash());
	if (nullptr == func)
	{
		std::cout << "Function '" << functionName << "' not found\n";
		return 1;
	}

	std::cout << "=== Running " << functionName << " ===\n";
	Runtime runtime;
	runtime.setProgram(program);
	runtime.setMemoryAccessHandler(&gMemory);
	runtime.callFunction(*func);

	SimConnector connector(runtime, hooks);
	size_t totalSteps = 0;
	try
	{
		while (!connector.mStopped)
		{
			runtime.executeSteps(connector, 100000, 0);
			totalSteps += connector.mStepsExecuted;
			if (connector.mResult == Runtime::ExecuteResult::Result::HALT)
				break;
			if (totalSteps > 500000000)
			{
				std::cout << "SIM ERROR: too many steps, endless loop?\n";
				return 1;
			}
		}
	}
	catch (const std::exception& e)
	{
		std::cout << "SIM ERROR: exception: " << e.what() << "\n";
		return 1;
	}
	std::cout << "=== Finished after " << totalSteps << " steps ===\n";
	return 0;
}

static int registerReferenceBindings(Module& module, const std::string& referenceFile)
{
	std::ifstream in(referenceFile);
	if (!in)
	{
		std::cerr << "Cannot open " << referenceFile << "\n";
		exit(2);
	}

	int count = 0;
	bool inOxygenCore = false;
	std::string line;
	while (std::getline(in, line))
	{
		line = trim(line);
		if (line.find("=== Module 'OxygenCore'") != std::string::npos)
		{
			inOxygenCore = true;
			continue;
		}
		if (!inOxygenCore)
			continue;

		if (line.rfind("declare function ", 0) == 0)
		{
			// declare function <ret> <name>(<params>)
			std::string rest = line.substr(17);
			const size_t space = rest.find(' ');
			const std::string returnTypeName = rest.substr(0, space);
			rest = rest.substr(space + 1);
			const size_t paren = rest.find('(');
			const std::string fullName = trim(rest.substr(0, paren));
			const std::string paramString = rest.substr(paren + 1, rest.rfind(')') - paren - 1);

			std::vector<const DataTypeDefinition*> params;
			std::stringstream ss(paramString);
			std::string param;
			while (std::getline(ss, param, ','))
			{
				param = trim(param);
				if (param.empty())
					continue;
				const size_t ps = param.find(' ');
				params.push_back(lookupType(param.substr(0, ps)));
			}

			if (gImplementedNatives.count(fullName + "/" + std::to_string(params.size())) != 0)
				continue;

			const size_t colon = fullName.find(':');
			if (colon != std::string::npos)
			{
				// Native methods get the object itself as implicit first parameter
				params.insert(params.begin(), lookupType(fullName.substr(0, colon)));
			}
			StubWrapper* wrapper = new StubWrapper(lookupType(returnTypeName), params);
			if (colon != std::string::npos)
			{
				module.addNativeMethod(FlyweightString(fullName.substr(0, colon)), FlyweightString(fullName.substr(colon + 1)), *wrapper);
			}
			else
			{
				module.addNativeFunction(FlyweightString(fullName), *wrapper);
			}
			++count;
		}
		else if (line.rfind("declare constant ", 0) == 0)
		{
			// declare constant <type> <name> = <value>
			std::string rest = line.substr(17);
			const size_t space = rest.find(' ');
			const DataTypeDefinition* type = lookupType(rest.substr(0, space));
			rest = rest.substr(space + 1);
			const size_t eq = rest.find('=');
			const std::string name = trim(rest.substr(0, eq));
			const std::string valueString = trim(rest.substr(eq + 1));
			AnyBaseValue value;
			if (type == &PredefinedDataTypes::FLOAT)
				value = AnyBaseValue(std::stof(valueString));
			else if (type == &PredefinedDataTypes::DOUBLE)
				value = AnyBaseValue(std::stod(valueString));
			else
				value = AnyBaseValue((int64)std::stoll(valueString, nullptr, 0));
			module.addConstant(FlyweightString(name), type, value);
			++count;
		}
	}
	return count;
}

static bool compileModule(Module& module, GlobalsLookup& globalsLookup, const std::string& filename)
{
	CompileOptions options;
	Compiler compiler(module, globalsLookup, options);
	bool success = false;
	try
	{
		success = compiler.loadScript(std::wstring(filename.begin(), filename.end()));
	}
	catch (const std::exception& e)
	{
		std::cout << "Exception while compiling " << filename << ": " << e.what() << "\n";
		return false;
	}

	if (success || compiler.getErrors().empty())
	{
		std::cout << "OK: " << filename << " (" << module.getScriptFunctions().size() << " script functions)\n";
		return true;
	}

	for (const Compiler::ErrorMessage& error : compiler.getErrors())
	{
		std::string file = "?";
		if (nullptr != error.mSourceFileInfo)
		{
			const std::wstring& w = error.mSourceFileInfo->mFilename;
			file = std::string(w.begin(), w.end());
		}
		std::cout << "ERROR: " << file << ":" << error.mError.mLineNumber << ": " << error.mMessage << "\n";
	}
	return false;
}

int main(int argc, char** argv)
{
	INIT_RMX;

	// Optional "--run <function>" at the end: execute that function after compiling
	std::string runFunction;
	if (argc >= 3 && std::string(argv[argc - 2]) == "--run")
	{
		runFunction = argv[argc - 1];
		argc -= 2;
	}

	if (argc < 3)
	{
		std::cerr << "Usage: lemoncheck <base main.lemon> <cpp_core_functions.lemon> [mod main.lemon...] [--run <function>]\n";
		return 2;
	}

	GlobalsLookup coreLookup;

	// LemonCore: standard library and preprocessor definitions as set by the S3AIR game app
	Module lemonCore("LemonCore");
	lemonCore.startCompiling(coreLookup);
	StandardLibrary::registerBindings(lemonCore);
	lemonCore.addPreprocessorDefinition(FlyweightString("STANDALONE"), 1);
	lemonCore.addPreprocessorDefinition(FlyweightString("GAMEAPP"), 0x26032800);
	coreLookup.addDefinitionsFromModule(lemonCore);

	// OxygenCore: engine and game bindings as stubs
	Module oxygenCore("OxygenCore");
	oxygenCore.startCompiling(coreLookup);
	{
		gTypes["void"]   = &PredefinedDataTypes::VOID;
		gTypes["any"]    = &PredefinedDataTypes::ANY;
		gTypes["bool"]   = &PredefinedDataTypes::BOOL;
		gTypes["u8"]     = &PredefinedDataTypes::UINT_8;
		gTypes["u16"]    = &PredefinedDataTypes::UINT_16;
		gTypes["u32"]    = &PredefinedDataTypes::UINT_32;
		gTypes["u64"]    = &PredefinedDataTypes::UINT_64;
		gTypes["s8"]     = &PredefinedDataTypes::INT_8;
		gTypes["s16"]    = &PredefinedDataTypes::INT_16;
		gTypes["s32"]    = &PredefinedDataTypes::INT_32;
		gTypes["s64"]    = &PredefinedDataTypes::INT_64;
		gTypes["float"]  = &PredefinedDataTypes::FLOAT;
		gTypes["double"] = &PredefinedDataTypes::DOUBLE;
		gTypes["string"] = &PredefinedDataTypes::STRING;
		// The return type of addCustomDataType differs between lemonscript versions (reference or pointer)
		gTypes["SpriteHandle"]  = asPointer(oxygenCore.addCustomDataType("SpriteHandle", BaseType::UINT_32));
		gTypes["AudioInstance"] = asPointer(oxygenCore.addCustomDataType("AudioInstance", BaseType::UINT_32));

		// Registers, as in LemonScriptBindings::registerBindings
		int64* registers = gRegisters;
		const std::string registerNames[16] = { "D0", "D1", "D2", "D3", "D4", "D5", "D6", "D7", "A0", "A1", "A2", "A3", "A4", "A5", "A6", "A7" };
		const std::pair<const char*, const DataTypeDefinition*> suffixes[] = {
			{ "",     &PredefinedDataTypes::UINT_32 },
			{ ".u8",  &PredefinedDataTypes::UINT_8 },
			{ ".s8",  &PredefinedDataTypes::INT_8 },
			{ ".u16", &PredefinedDataTypes::UINT_16 },
			{ ".s16", &PredefinedDataTypes::INT_16 },
			{ ".u32", &PredefinedDataTypes::UINT_32 },
			{ ".s32", &PredefinedDataTypes::INT_32 },
		};
		for (int i = 0; i < 16; ++i)
		{
			for (const auto& suffix : suffixes)
			{
				int64* ptr = &registers[i];
				oxygenCore.addExternalVariable(FlyweightString(registerNames[i] + suffix.first), suffix.second, [ptr]() { return ptr; });
			}
		}
		oxygenCore.addUserDefinedVariable(FlyweightString("Log"), &PredefinedDataTypes::ANY);
		oxygenCore.addUserDefinedVariable(FlyweightString("LogDec"), &PredefinedDataTypes::ANY);
		for (int i = 0; i < 10; ++i)
			oxygenCore.addUserDefinedVariable(FlyweightString("Key" + std::to_string(i)), &PredefinedDataTypes::UINT_8);

		registerSimulationNatives(oxygenCore);
		const int count = registerReferenceBindings(oxygenCore, argv[2]);
		std::cout << "Registered " << count << " engine bindings as stubs\n";
	}
	coreLookup.addDefinitionsFromModule(oxygenCore);

	// Base game scripts
	GlobalsLookup lookup = coreLookup;
	Module gameScript("GameScript");
	if (!compileModule(gameScript, lookup, argv[1]))
		return 1;

	// Mods, in order, each one seeing the previous modules
	Module* previous = &gameScript;
	std::vector<Module*> mods;
	for (int i = 3; i < argc; ++i)
	{
		lookup.addDefinitionsFromModule(*previous);
		Module* mod = new Module("mod" + std::to_string(i - 2));
		if (!compileModule(*mod, lookup, argv[i]))
			return 1;
		mods.push_back(mod);
		previous = mod;
	}

	if (!runFunction.empty())
	{
		std::vector<const Module*> modules = { &lemonCore, &oxygenCore, &gameScript };
		for (Module* mod : mods)
			modules.push_back(mod);
		return runSimulation(modules, runFunction);
	}
	return 0;
}

#pragma once
#include "pch.h"
#include "SNES/AluMulDiv.h"
#include "SNES/SnesCpu.h"
#include "SNES/SnesPpu.h"
#include "SNES/InternalRegisterTypes.h"
#include "Utilities/ISerializable.h"

class SnesConsole;
class SnesMemoryManager;
class SnesControlManager;

class InternalRegisters final : public ISerializable
{
private:
	SnesConsole* _console = nullptr;
	SnesCpu* _cpu = nullptr;
	SnesPpu* _ppu = nullptr;
	SnesMemoryManager* _memoryManager = nullptr;
	SnesControlManager* _controlManager = nullptr;

	AluMulDiv _aluMulDiv = {};

	InternalRegisterState _state = {};
	bool _nmiFlag = false;
	bool _irqLevel = false;
	uint8_t _needIrq = 0;
	bool _irqFlag = false;

	uint64_t _autoReadClockStart = 0;
	uint64_t _autoReadNextClock = 0;
	bool _autoReadActive = false;
	bool _autoReadDisabled = true;
	uint8_t _autoReadPort1Value = 0;
	uint8_t _autoReadPort2Value = 0;

	void SetIrqFlag(bool irqFlag);
	
	uint8_t ReadControllerData(uint8_t port, bool getMsb);

public:
	InternalRegisters();
	void Initialize(SnesConsole* console);

	void Reset();

	void SetAutoJoypadReadClock();
	void ProcessAutoJoypad();

	__forceinline void ProcessIrqCounters();

	uint8_t GetIoPortOutput();
	void SetNmiFlag(bool nmiFlag);

	bool IsVerticalIrqEnabled() { return _state.EnableVerticalIrq; }
	bool IsHorizontalIrqEnabled() { return _state.EnableHorizontalIrq; }
	bool IsNmiEnabled() { return _state.EnableNmi; }
	bool IsFastRomEnabled() { return _state.EnableFastRom; }
	uint16_t GetHorizontalTimer() { return _state.HorizontalTimer; }
	uint16_t GetVerticalTimer() { return _state.VerticalTimer; }
	
	uint8_t Peek(uint16_t addr);
	uint8_t Read(uint16_t addr);
	void Write(uint16_t addr, uint8_t value);

	InternalRegisterState GetState();
	AluState GetAluState();

	void Serialize(Serializer &s) override;
};

void InternalRegisters::ProcessIrqCounters()
{
	if(_needIrq > 0) {
		_needIrq--;
		if(_needIrq == 0) {
			SetIrqFlag(true);
		}
	}

	bool irqLevel = (
		(_state.EnableHorizontalIrq || _state.EnableVerticalIrq) &&
		(!_state.EnableHorizontalIrq || (_state.HorizontalTimer <= 339 && (_ppu->GetCycle() == _state.HorizontalTimer) && (_ppu->GetLastScanline() != _ppu->GetRealScanline() || _state.HorizontalTimer < 339))) &&
		(!_state.EnableVerticalIrq || _ppu->GetRealScanline() == _state.VerticalTimer)
	);

	if(!_irqLevel && irqLevel) {
		//Trigger IRQ signal 16 master clocks later
		_needIrq = 4;
	}
	_irqLevel = irqLevel;
	_cpu->SetNmiFlag(_state.EnableNmi & _nmiFlag);
}
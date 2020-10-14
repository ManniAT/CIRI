#pragma once
#include "./FSMState.h"

class FSMValues {
protected:
	short _PerfectMax;
	short _GoodMax;
	short _BadMax;
	short _PerfectReset;
	short _GoodReset;
	short _BadReset;
public:
	FSMValues(short pPerfectMax, short pGoodMax, short pBadMax, short pGood2PerfectDelta, short pBad2GoodDelta, short pAlert2BadDelta) {
		_PerfectMax = pPerfectMax;
		_GoodMax = pGoodMax;
		_BadMax = pBadMax;
		_PerfectReset = _PerfectMax - pGood2PerfectDelta;
		_GoodReset = _GoodMax - pBad2GoodDelta;
		_BadReset = _BadMax - pAlert2BadDelta;
	}
	FSMValues(short pPerfectMax, short pGoodMax, short pBadMax, short pDownStepDelta) : FSMValues(pPerfectMax, pGoodMax, pBadMax, pDownStepDelta, pDownStepDelta, pDownStepDelta) {}
};

class FSM : FSMValues {
private:
	FSMState _CurState;
	short _CurValue;
	void Init() {
		_CurState = FSMState::UNKNOWN;
		_CurValue = 0;
	}
	FSMState GetValueStatue(short pValue) {
		if (pValue > _BadMax) {
			return(FSMState::ALERT);
		}
		if (pValue > _GoodMax) {
			return(FSMState::BAD);
		}
		if (pValue > _PerfectMax) {
			return(FSMState::GOOD);
		}
		return(FSMState::PERFECT);
	}
public:
	const FSMState& CurState = _CurState;
	FSM(FSMValues pValues) : FSMValues(pValues) { Init(); }
	FSM(short pPerfectMax, short pGoodMax, short pBadMax, short pDownStepDelta) : FSMValues(pPerfectMax, pGoodMax, pBadMax, pDownStepDelta, pDownStepDelta, pDownStepDelta) { Init(); }
	FSM(short pPerfectMax, short pGoodMax, short pBadMax, short pGood2PerfectDelta, short pBad2GoodDelta, short pAlert2BadDelta)
		:FSMValues(pPerfectMax, pGoodMax, pBadMax, pGood2PerfectDelta, pBad2GoodDelta, pAlert2BadDelta) {
		Init();
	}
	bool SetValue(short pNewValue);
};
bool FSM::SetValue(short pNewValue) {
	if (_CurValue == pNewValue) {
		return(false);	//nothing changed
	}
	_CurValue = pNewValue;
	FSMState sForNewValue = GetValueStatue(pNewValue);
	if (sForNewValue == _CurState) {	//still the same
		return(false);
	}
	if (sForNewValue > _CurState) {	//becoming worse (or UNKNOWN >> GOOD)- no check needed
		_CurState = sForNewValue;
		return(true);
	}
	if (_CurValue <= _PerfectReset) {	//going down - check for reset (hysteresis) from top to bottom (can bypass some states if going down fast) 
		_CurState = sForNewValue;
		return(true);
	}
	if (_CurValue <= _GoodReset) {
		_CurState = sForNewValue;
		return(true);
	}
	if (_CurValue <= _BadReset) {
		_CurState = sForNewValue;
		return(true);
	}
	return(false);	//not going down due to hysteresis
}
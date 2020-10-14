#pragma once
class FSMState {
public:
	enum State : byte {
		UNKNOWN = 0, PERFECT = 1, GOOD = 2, BAD = 3, ALERT = 4
	};
	FSMState() : FSMState(FSMState::UNKNOWN) {}
	constexpr FSMState(State pState) : _State(pState) { }
private:
	State _State;
public:
	constexpr bool operator==(FSMState a) const { return _State == a._State; }
	constexpr bool operator!=(FSMState a) const { return _State != a._State; }
	constexpr bool operator>(FSMState a) const { return (byte)_State > (byte)a._State; }
	constexpr bool operator<(FSMState a) const { return (byte)_State < (byte)a._State; }
	constexpr bool operator>=(FSMState a) const { return (byte)_State >= (byte)a._State; }
	constexpr bool operator<=(FSMState a) const { return (byte)_State <= (byte)a._State; }
	FSMState& operator=(const FSMState& a) { _State = a._State; return(*this); }
	operator byte() const { return((byte)_State); }
};

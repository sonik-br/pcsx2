// SPDX-FileCopyrightText: 2002-2025 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#include "usb-pad.h"
#include "IconsFontAwesome6.h"
#include "IconsPromptFont.h"
#include "USB/qemu-usb/USBinternal.h"
#include "USB/usb-pad/usb-pad-sdl-ff.h"
#include "USB/USB.h"
#include "Host.h"
#include "StateWrapper.h"

#include "common/Console.h"

namespace usb_pad
{
	static const USBDescStrings df_desc_strings = {
		"",
		"Logitech Driving Force",
		"",
		"Logitech",
	};

	static const USBDescStrings dfp_desc_strings = {
		"",
		"Logitech Driving Force Pro",
		"",
		"Logitech",
	};

	static const USBDescStrings gtf_desc_strings = {
		"",
		"Logitech", //actual index @ 0x04
		"Logitech GT Force" //actual index @ 0x20
	};

	static const USBDescStrings rb1_desc_strings = {
		"1234567890AB",
		"Licensed by Sony Computer Entertainment America",
		"Harmonix Drum Kit for PlayStation(R)3"};

	static const USBDescStrings kbm_desc_strings = {
		"",
		"USB Multipurpose Controller",
		"",
		"KONAMI"};

	static const USBDescStrings flightforce_desc_strings = {
		"",
		"Logitech Inc.", //actual index @ 0x04
		"WingMan Force 3D" //actual index @ 0x20
	};

	static int MapSteeringCurveExponentOptionToExponent(const std::string& option)
	{
		int exponent = 0;
		if (option == "Low")
		{
			exponent = 1;
		}
		else if (option == "Medium")
		{
			exponent = 2;
		}
		else if (option == "High")
		{
			exponent = 3;
		}

		return exponent;
	}

	static std::span<const InputBindingInfo> GetWheelBindings(PS2WheelTypes wt)
	{
		switch (wt)
		{
			case WT_GENERIC:
			{
				static constexpr const InputBindingInfo bindings[] = {
					{"SteeringLeft", TRANSLATE_NOOP("USB", "Steering Left"), nullptr, InputBindingInfo::Type::HalfAxis, CID_STEERING_L, GenericInputBinding::LeftStickLeft},
					{"SteeringRight", TRANSLATE_NOOP("USB", "Steering Right"), nullptr, InputBindingInfo::Type::HalfAxis, CID_STEERING_R, GenericInputBinding::LeftStickRight},
					{"Throttle", TRANSLATE_NOOP("USB", "Throttle"), nullptr, InputBindingInfo::Type::HalfAxis, CID_THROTTLE, GenericInputBinding::R2},
					{"Brake", TRANSLATE_NOOP("USB", "Brake"), nullptr, InputBindingInfo::Type::HalfAxis, CID_BRAKE, GenericInputBinding::L2},
					{"DPadUp", TRANSLATE_NOOP("USB", "D-Pad Up"), nullptr, InputBindingInfo::Type::Button, CID_DPAD_UP, GenericInputBinding::DPadUp},
					{"DPadDown", TRANSLATE_NOOP("USB", "D-Pad Down"), nullptr, InputBindingInfo::Type::Button, CID_DPAD_DOWN, GenericInputBinding::DPadDown},
					{"DPadLeft", TRANSLATE_NOOP("USB", "D-Pad Left"), nullptr, InputBindingInfo::Type::Button, CID_DPAD_LEFT, GenericInputBinding::DPadLeft},
					{"DPadRight", TRANSLATE_NOOP("USB", "D-Pad Right"), nullptr, InputBindingInfo::Type::Button, CID_DPAD_RIGHT, GenericInputBinding::DPadRight},
					{"Cross", TRANSLATE_NOOP("USB", "Cross"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON0, GenericInputBinding::Cross},
					{"Square", TRANSLATE_NOOP("USB", "Square"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON1, GenericInputBinding::Square},
					{"Circle", TRANSLATE_NOOP("USB", "Circle"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON2, GenericInputBinding::Circle},
					{"Triangle", TRANSLATE_NOOP("USB", "Triangle"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON3, GenericInputBinding::Triangle},
					{"L1", TRANSLATE_NOOP("USB", "L1"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON5, GenericInputBinding::L1},
					{"R1", TRANSLATE_NOOP("USB", "R1"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON4, GenericInputBinding::R1},
					{"L2", TRANSLATE_NOOP("USB", "L2"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON7, GenericInputBinding::Unknown}, // used L2 for brake
					{"R2", TRANSLATE_NOOP("USB", "R2"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON6, GenericInputBinding::Unknown}, // used R2 for throttle
					{"Select", TRANSLATE_NOOP("USB", "Select"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON8, GenericInputBinding::Select},
					{"Start", TRANSLATE_NOOP("USB", "Start"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON9, GenericInputBinding::Start},
					{"FFDevice", TRANSLATE_NOOP("USB", "Force Feedback"), nullptr, InputBindingInfo::Type::Device, 0, GenericInputBinding::Unknown},
				};

				return bindings;
			}

			case WT_DRIVING_FORCE_PRO:
			case WT_DRIVING_FORCE_PRO_1102:
			{
				static constexpr const InputBindingInfo bindings[] = {
					{"SteeringLeft", TRANSLATE_NOOP("USB", "Steering Left"), nullptr, InputBindingInfo::Type::HalfAxis, CID_STEERING_L, GenericInputBinding::LeftStickLeft},
					{"SteeringRight", TRANSLATE_NOOP("USB", "Steering Right"), nullptr, InputBindingInfo::Type::HalfAxis, CID_STEERING_R, GenericInputBinding::LeftStickRight},
					{"Throttle", TRANSLATE_NOOP("USB", "Throttle"), nullptr, InputBindingInfo::Type::HalfAxis, CID_THROTTLE, GenericInputBinding::R2},
					{"Brake", TRANSLATE_NOOP("USB", "Brake"), nullptr, InputBindingInfo::Type::HalfAxis, CID_BRAKE, GenericInputBinding::L2},
					{"DPadUp", TRANSLATE_NOOP("USB", "D-Pad Up"), nullptr, InputBindingInfo::Type::Button, CID_DPAD_UP, GenericInputBinding::DPadUp},
					{"DPadDown", TRANSLATE_NOOP("USB", "D-Pad Down"), nullptr, InputBindingInfo::Type::Button, CID_DPAD_DOWN, GenericInputBinding::DPadDown},
					{"DPadLeft", TRANSLATE_NOOP("USB", "D-Pad Left"), nullptr, InputBindingInfo::Type::Button, CID_DPAD_LEFT, GenericInputBinding::DPadLeft},
					{"DPadRight", TRANSLATE_NOOP("USB", "D-Pad Right"), nullptr, InputBindingInfo::Type::Button, CID_DPAD_RIGHT, GenericInputBinding::DPadRight},
					{"Cross", TRANSLATE_NOOP("USB", "Cross"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON0, GenericInputBinding::Cross},
					{"Square", TRANSLATE_NOOP("USB", "Square"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON1, GenericInputBinding::Square},
					{"Circle", TRANSLATE_NOOP("USB", "Circle"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON2, GenericInputBinding::Circle},
					{"Triangle", TRANSLATE_NOOP("USB", "Triangle"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON3, GenericInputBinding::Triangle},
					{"R1", TRANSLATE_NOOP("USB", "Shift Up / R1"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON4, GenericInputBinding::R1},
					{"L1", TRANSLATE_NOOP("USB", "Shift Down / L1"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON5, GenericInputBinding::L1},
					{"Select", TRANSLATE_NOOP("USB", "Select"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON8, GenericInputBinding::Select},
					{"Start", TRANSLATE_NOOP("USB", "Start"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON9, GenericInputBinding::Start},
					{"L2", TRANSLATE_NOOP("USB", "L2"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON7, GenericInputBinding::Unknown}, // used L2 for brake
					{"R2", TRANSLATE_NOOP("USB", "R2"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON6, GenericInputBinding::Unknown}, // used R2 for throttle
					{"L3", TRANSLATE_NOOP("USB", "L3"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON11, GenericInputBinding::L3},
					{"R3", TRANSLATE_NOOP("USB", "R3"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON10, GenericInputBinding::R3},
					{"FFDevice", "Force Feedback", nullptr, InputBindingInfo::Type::Device, 0, GenericInputBinding::Unknown},
				};

				return bindings;
			}

			case WT_GT_FORCE:
			{
				static constexpr const InputBindingInfo bindings[] = {
					{"SteeringLeft", TRANSLATE_NOOP("USB", "Steering Left"), nullptr, InputBindingInfo::Type::HalfAxis, CID_STEERING_L, GenericInputBinding::LeftStickLeft},
					{"SteeringRight", TRANSLATE_NOOP("USB", "Steering Right"), nullptr, InputBindingInfo::Type::HalfAxis, CID_STEERING_R, GenericInputBinding::LeftStickRight},
					{"Throttle", TRANSLATE_NOOP("USB", "Throttle"), nullptr, InputBindingInfo::Type::HalfAxis, CID_THROTTLE, GenericInputBinding::R2},
					{"Brake", TRANSLATE_NOOP("USB", "Brake"), nullptr, InputBindingInfo::Type::HalfAxis, CID_BRAKE, GenericInputBinding::L2},
					{"MenuUp", TRANSLATE_NOOP("USB", "Menu Up"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON1, GenericInputBinding::DPadUp},
					{"MenuDown", TRANSLATE_NOOP("USB", "Menu Down"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON0, GenericInputBinding::DPadDown},
					{"X", TRANSLATE_NOOP("USB", "X"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON2, GenericInputBinding::Square},
					{"Y", TRANSLATE_NOOP("USB", "Y"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON3, GenericInputBinding::Triangle},
					{"A", TRANSLATE_NOOP("USB", "A"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON4, GenericInputBinding::Cross},
					{"B", TRANSLATE_NOOP("USB", "B"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON5, GenericInputBinding::Circle},
					{"FFDevice", TRANSLATE_NOOP("USB", "Force Feedback"), nullptr, InputBindingInfo::Type::Device, 0, GenericInputBinding::Unknown},
				};

				return bindings;
			}

			default:
			{
				return {};
			}
		}
	}

	static std::span<const SettingInfo> GetWheelSettings(PS2WheelTypes wt)
	{
		if (wt <= WT_GT_FORCE)
		{
			static constexpr const char* SteeringCurveExponentOptions[] = {TRANSLATE_NOOP("USB", "Off"), TRANSLATE_NOOP("USB", "Low"), TRANSLATE_NOOP("USB", "Medium"), TRANSLATE_NOOP("USB", "High"), nullptr};
			static constexpr const SettingInfo info[] = {
				{SettingInfo::Type::Integer, "SteeringSmoothing", TRANSLATE_NOOP("USB", "Steering Smoothing"),
					TRANSLATE_NOOP("USB", "Smooths out changes in steering to the specified percentage per poll. Needed for using keyboards."),
					"0", "0", "100", "1", TRANSLATE_NOOP("USB", "%d%%"), nullptr, nullptr, 1.0f},
				{SettingInfo::Type::Integer, "SteeringDeadzone", TRANSLATE_NOOP("USB", "Steering Deadzone"),
					TRANSLATE_NOOP("USB", "Steering axis deadzone for pads or non self centering wheels."),
					"0", "0", "100", "1", TRANSLATE_NOOP("USB", "%d%%"), nullptr, nullptr, 1.0f},
				{SettingInfo::Type::StringList, "SteeringCurveExponent", TRANSLATE_NOOP("USB", "Steering Damping"),
					TRANSLATE_NOOP("USB", "Applies power curve filter to steering axis values. Dampens small inputs."),
					"Off", nullptr, nullptr, nullptr, nullptr, SteeringCurveExponentOptions},
				{SettingInfo::Type::Boolean, "FfbDropoutWorkaround", TRANSLATE_NOOP("USB", "Workaround for Intermittent FFB Loss"),
					TRANSLATE_NOOP("USB", "Works around bugs in some wheels' firmware that result in brief interruptions in force. Leave this disabled unless you need it, as it has negative side effects on many wheels."),
					"false"}
			};

			return info;
		}
		else
		{
			return {};
		}
	}

	PadState::PadState(u32 port_, PS2WheelTypes type_)
		: port(port_)
		, type(type_)
	{
		if (type_ == WT_DRIVING_FORCE_PRO || type_ == WT_DRIVING_FORCE_PRO_1102)
			steering_range = 0x3FFF >> 1;
		else if (type_ == WT_SEGA_SEAMIC)
			steering_range = 0xFF >> 1;
		else
			steering_range = 0x3FF >> 1;

		steering_step = std::numeric_limits<u16>::max();

		// steering starts in the center
		data.last_steering = steering_range;
		data.steering = steering_range;

		// throttle/brake start unpressed
		data.throttle = 255;
		data.brake = 255;

		//flight stick
		data.stick_x = 0x80;
		data.stick_y = 0x80;
		data.stick_throttle = 0x80;
		data.stick_rudder = 0x80;
		data.stick_hat_x = 0x80;
		data.stick_hat_y = 0x80;

		Reset();
	}

	PadState::~PadState() = default;

	void PadState::UpdateSettings(SettingsInterface& si, const char* devname)
	{
		const s32 smoothing_percent = USB::GetConfigInt(si, port, devname, "SteeringSmoothing", 0);
		if (smoothing_percent <= 0)
		{
			// none, allow any amount of change
			steering_step = std::numeric_limits<u16>::max();
		}
		else
		{
			steering_step = static_cast<u16>(std::clamp<s32>((steering_range * smoothing_percent) / 100,
				1, std::numeric_limits<u16>::max()));
		}

		steering_deadzone = (steering_range * USB::GetConfigInt(si, port, devname, "SteeringDeadzone", 0)) / 100;
		steering_curve_exponent = MapSteeringCurveExponentOptionToExponent(USB::GetConfigString(si, port, devname, "SteeringCurveExponent", "Off"));

		if (HasFF())
		{
			const std::string ffdevname(USB::GetConfigString(si, port, devname, "FFDevice"));
			if (ffdevname != mFFdevName)
			{
				mFFdev.reset();
				mFFdevName = std::move(ffdevname);
				OpenFFDevice();
			}
			if (mFFdev != NULL)
			{
				const bool use_ffb_dropout_workaround = USB::GetConfigBool(si, port, devname, "FfbDropoutWorkaround", false);
				mFFdev->use_ffb_dropout_workaround = use_ffb_dropout_workaround;
			}
		}
	}

	void PadState::Reset()
	{
		data.steering = steering_range;
		mFFstate = {};
	}

	int PadState::TokenIn(u8* buf, int len)
	{
		// TODO: This is still pretty gross and needs cleaning up.

		struct wheel_lohi
		{
			u32 lo;
			u32 hi;
		};

		wheel_lohi* w = reinterpret_cast<wheel_lohi*>(buf);
		std::memset(w, 0, 8);

		switch (type)
		{
			case WT_GENERIC:
			{
				UpdateSteering();
				UpdateHatSwitch();

				DbgCon.WriteLn("Steering: %d Throttle: %d Brake: %d Buttons: %d",
					data.steering, data.throttle, data.brake, data.buttons);

				w->lo = data.steering & 0x3FF;
				w->lo |= (data.buttons & 0xFFF) << 10;
				w->lo |= 0xFF << 24;

				w->hi = (data.hatswitch & 0xF);
				w->hi |= (data.throttle & 0xFF) << 8;
				w->hi |= (data.brake & 0xFF) << 16;

				return len;
			}

			case WT_GT_FORCE:
			{
				UpdateSteering();
				UpdateHatSwitch();

				w->lo = data.steering & 0x3FF;
				w->lo |= (data.buttons & 0xFFF) << 10;
				w->lo |= 1 << 16; // Tokyo Xtreme Racer (Zero) ignores the pedals unless the 3rd byte is different than zero
				w->lo |= 0xFF << 24;

				w->hi = (data.throttle & 0xFF);
				w->hi |= (data.brake & 0xFF) << 8;

				return len;
			}

			case WT_DRIVING_FORCE_PRO:
			{
				UpdateSteering();
				UpdateHatSwitch();

				w->lo = data.steering & 0x3FFF;
				w->lo |= (data.buttons & 0x3FFF) << 14;
				w->lo |= (data.hatswitch & 0xF) << 28;

				w->hi = 0x00;
				w->hi |= data.throttle << 8;
				w->hi |= data.brake << 16; //axis_rz
				w->hi |= 0x11 << 24; //enables wheel and pedals?

				return len;
			}

			case WT_DRIVING_FORCE_PRO_1102:
			{
				UpdateSteering();
				UpdateHatSwitch();

				// what's up with the bitmap?
				// xxxxxxxx xxxxxxbb bbbbbbbb bbbbhhhh ???????? ?01zzzzz 1rrrrrr1 10001000
				w->lo = data.steering & 0x3FFF;
				w->lo |= (data.buttons & 0x3FFF) << 14;
				w->lo |= (data.hatswitch & 0xF) << 28;

				w->hi = 0x00;
				//w->hi |= 0 << 9; //bit 9 must be 0
				w->hi |= (1 | (data.throttle * 0x3F) / 0xFF) << 10; //axis_z
				w->hi |= 1 << 16; //bit 16 must be 1
				w->hi |= ((0x3F - (data.brake * 0x3F) / 0xFF) & 0x3F) << 17; //axis_rz
				w->hi |= 1 << 23; //bit 23 must be 1
				w->hi |= 0x11 << 24; //enables wheel and pedals?

				return len;
			}

			case WT_ROCKBAND1_DRUMKIT:
			{
				UpdateHatSwitch();

				w->lo = (data.buttons & 0xFFF);
				w->lo |= (data.hatswitch & 0xF) << 16;

				return len;
			}

			case WT_SEGA_SEAMIC:
			{
				UpdateSteering();
				UpdateHatSwitch();

				buf[0] = data.steering & 0xFF;
				buf[1] = data.throttle & 0xFF;
				buf[2] = data.brake & 0xFF;
				buf[3] = data.hatswitch & 0x0F; // 4bits?
				buf[3] |= (data.buttons & 0x0F) << 4; // 4 bits // TODO Or does it start at buf[4]?
				buf[4] = (data.buttons >> 4) & 0x3F; // 10 - 4 = 6 bits

				return len;
			}

			case WT_KEYBOARDMANIA_CONTROLLER:
			{
				buf[0] = 0x3F;
				buf[1] = data.buttons & 0xFF;
				buf[2] = (data.buttons >> 8) & 0xFF;
				buf[3] = (data.buttons >> 16) & 0xFF;
				buf[4] = (data.buttons >> 24) & 0xFF;

				return len;
			}

			case WT_FLIGHTSTICK_FS1:
			case WT_FLIGHTSTICK_FS2:
			{
				UpdateStickX();
				UpdateStickY();
				UpdateStickThrottle();
				UpdateStickRudder();
				UpdateStickHatX();
				UpdateStickHatY();

				buf[0] = data.stick_x;
				buf[1] = data.stick_y;
				buf[2] = data.stick_rudder;
				buf[3] = data.stick_throttle;
				buf[4] = data.stick_hat_x;
				buf[5] = data.stick_hat_y;
				buf[6] = data.brake; // Triangle (A)
				buf[7] = data.throttle; // Square (B)

				return len;
			}

			case WT_FLIGHTSTICK_FLIGHTFORCE:
			{
				UpdateHatSwitch();
				UpdateStickX();
				UpdateStickY();
				UpdateStickThrottle();
				UpdateStickRudder();

				buf[0] = data.stick_x;
				buf[1] = data.stick_y;
				buf[2] = static_cast<u8>(data.hatswitch << 4);
				buf[3] = data.stick_rudder;
				buf[4] = static_cast<u8>(data.buttons);
				buf[5] = static_cast<u8>(~data.stick_throttle);
				buf[6] = 0x03;

				return len;
			}

			default:
			{
				return len;
			}
		}
	}

	int PadState::TokenOut(const u8* data, int len)
	{
		const ff_data* ffdata = reinterpret_cast<const ff_data*>(data);
		bool hires = (type == WT_DRIVING_FORCE_PRO || type == WT_DRIVING_FORCE_PRO_1102);
		ParseFFData(ffdata, hires);
		return len;
	}

	float PadState::GetBindValue(u32 bind_index) const
	{
		switch (bind_index)
		{
			case CID_STEERING_L:
				return static_cast<float>(data.steering_left) / static_cast<float>(steering_range);

			case CID_STEERING_R:
				return static_cast<float>(data.steering_right) / static_cast<float>(steering_range);

			case CID_THROTTLE:
				return 1.0f - (static_cast<float>(data.throttle) / 255.0f);

			case CID_BRAKE:
				return 1.0f - (static_cast<float>(data.brake) / 255.0f);

			case CID_DPAD_UP:
				return static_cast<float>(data.hat_up);

			case CID_DPAD_DOWN:
				return static_cast<float>(data.hat_down);

			case CID_DPAD_LEFT:
				return static_cast<float>(data.hat_left);

			case CID_DPAD_RIGHT:
				return static_cast<float>(data.hat_right);

			case CID_BUTTON0:
			case CID_BUTTON1:
			case CID_BUTTON2:
			case CID_BUTTON3:
			case CID_BUTTON5:
			case CID_BUTTON4:
			case CID_BUTTON7:
			case CID_BUTTON6:
			case CID_BUTTON8:
			case CID_BUTTON9:
			case CID_BUTTON10:
			case CID_BUTTON11:
			case CID_BUTTON12:
			case CID_BUTTON13:
			case CID_BUTTON14:
			case CID_BUTTON15:
			case CID_BUTTON16:
			case CID_BUTTON17:
			case CID_BUTTON18:
			case CID_BUTTON19:
			case CID_BUTTON20:
			case CID_BUTTON21:
			case CID_BUTTON22:
			case CID_BUTTON23:
			case CID_BUTTON24:
			case CID_BUTTON25:
			case CID_BUTTON26:
			case CID_BUTTON27:
			case CID_BUTTON28:
			case CID_BUTTON29:
			case CID_BUTTON30:
			case CID_BUTTON31:
			{
				const u32 mask = (1u << (bind_index - CID_BUTTON0));
				return ((data.buttons & mask) != 0u) ? 1.0f : 0.0f;
			}

			case CID_STICK_L:
				return static_cast<float>(data.stick_left) / static_cast<float>(stick_range);

			case CID_STICK_R:
				return static_cast<float>(data.stick_right) / static_cast<float>(stick_range);

			case CID_STICK_U:
				return static_cast<float>(data.stick_up) / static_cast<float>(stick_range);

			case CID_STICK_D:
				return static_cast<float>(data.stick_down) / static_cast<float>(stick_range);

			case CID_STICK_THROTTLE_U:
				return static_cast<float>(data.stick_throttle_up) / static_cast<float>(stick_range);

			case CID_STICK_THROTTLE_D:
				return static_cast<float>(data.stick_throttle_down) / static_cast<float>(stick_range);

			case CID_STICK_RUDDER_L:
				return static_cast<float>(data.stick_rudder_left) / static_cast<float>(stick_range);

			case CID_STICK_RUDDER_R:
				return static_cast<float>(data.stick_rudder_left) / static_cast<float>(stick_range);

			case CID_STICK_HAT_L:
				return static_cast<float>(data.stick_hat_left) / static_cast<float>(stick_range);

			case CID_STICK_HAT_R:
				return static_cast<float>(data.stick_hat_right) / static_cast<float>(stick_range);

			case CID_STICK_HAT_U:
				return static_cast<float>(data.stick_hat_up) / static_cast<float>(stick_range);

			case CID_STICK_HAT_D:
				return static_cast<float>(data.stick_hat_down) / static_cast<float>(stick_range);

			default:
				return 0.0f;
		}
	}

	s16 PadState::ApplySteeringAxisModifiers(float value)
	{
		const s16 raw_steering = static_cast<s16>(std::lroundf(value * static_cast<float>(steering_range)));
		const s16 deadzone_offset = static_cast<s16>(std::lroundf(value * static_cast<float>(steering_deadzone)));
		const s16 deadzone_modified_steering = std::max((raw_steering - steering_deadzone + deadzone_offset), 0);

		if (steering_curve_exponent)
		{
			return std::pow(deadzone_modified_steering, steering_curve_exponent + 1) / std::pow(steering_range, steering_curve_exponent);
		}
		else
		{
			return deadzone_modified_steering;
		}
	}

	void PadState::SetBindValue(u32 bind_index, float value)
	{
		switch (bind_index)
		{
			case CID_STEERING_L:
				data.steering_left = ApplySteeringAxisModifiers(value);
				UpdateSteering();
				break;

			case CID_STEERING_R:
				data.steering_right = ApplySteeringAxisModifiers(value);
				UpdateSteering();
				break;

			case CID_THROTTLE:
				data.throttle = static_cast<u32>(255 - std::clamp<long>(std::lroundf(value * 255.0f), 0, 255));
				break;

			case CID_BRAKE:
				data.brake = static_cast<u32>(255 - std::clamp<long>(std::lroundf(value * 255.0f), 0, 255));
				break;

			case CID_DPAD_UP:
				data.hat_up = static_cast<u8>(std::clamp<long>(std::lroundf(value * 255.0f), 0, 255));
				UpdateHatSwitch();
				break;
			case CID_DPAD_DOWN:
				data.hat_down = static_cast<u8>(std::clamp<long>(std::lroundf(value * 255.0f), 0, 255));
				UpdateHatSwitch();
				break;
			case CID_DPAD_LEFT:
				data.hat_left = static_cast<u8>(std::clamp<long>(std::lroundf(value * 255.0f), 0, 255));
				UpdateHatSwitch();
				break;
			case CID_DPAD_RIGHT:
				data.hat_right = static_cast<u8>(std::clamp<long>(std::lroundf(value * 255.0f), 0, 255));
				UpdateHatSwitch();
				break;

			case CID_BUTTON0:
			case CID_BUTTON1:
			case CID_BUTTON2:
			case CID_BUTTON3:
			case CID_BUTTON5:
			case CID_BUTTON4:
			case CID_BUTTON7:
			case CID_BUTTON6:
			case CID_BUTTON8:
			case CID_BUTTON9:
			case CID_BUTTON10:
			case CID_BUTTON11:
			case CID_BUTTON12:
			case CID_BUTTON13:
			case CID_BUTTON14:
			case CID_BUTTON15:
			case CID_BUTTON16:
			case CID_BUTTON17:
			case CID_BUTTON18:
			case CID_BUTTON19:
			case CID_BUTTON20:
			case CID_BUTTON21:
			case CID_BUTTON22:
			case CID_BUTTON23:
			case CID_BUTTON24:
			case CID_BUTTON25:
			case CID_BUTTON26:
			case CID_BUTTON27:
			case CID_BUTTON28:
			case CID_BUTTON29:
			case CID_BUTTON30:
			case CID_BUTTON31:
			{
				const u32 mask = (1u << (bind_index - CID_BUTTON0));
				if (value >= 0.5f)
					data.buttons |= mask;
				else
					data.buttons &= ~mask;
			}
			break;

			case CID_STICK_L:
				data.stick_left = static_cast<u8>(std::clamp<long>(std::lroundf(value * 255.0f), 0, 255));
				UpdateStickX();
				break;

			case CID_STICK_R:
				data.stick_right = static_cast<u8>(std::clamp<long>(std::lroundf(value * 255.0f), 0, 255));
				UpdateStickX();
				break;

			case CID_STICK_U:
				data.stick_up = static_cast<u8>(std::clamp<long>(std::lroundf(value * 255.0f), 0, 255));
				UpdateStickY();
				break;

			case CID_STICK_D:
				data.stick_down = static_cast<u8>(std::clamp<long>(std::lroundf(value * 255.0f), 0, 255));
				UpdateStickY();
				break;

			case CID_STICK_THROTTLE_U:
				data.stick_throttle_up = static_cast<u8>(std::clamp<long>(std::lroundf(value * 255.0f), 0, 255));
				UpdateStickThrottle();
				break;

			case CID_STICK_THROTTLE_D:
				data.stick_throttle_down = static_cast<u8>(std::clamp<long>(std::lroundf(value * 255.0f), 0, 255));
				UpdateStickThrottle();
				break;

			case CID_STICK_RUDDER_L:
				data.stick_rudder_left = static_cast<u8>(std::clamp<long>(std::lroundf(value * 255.0f), 0, 255));
				UpdateStickRudder();
				break;

			case CID_STICK_RUDDER_R:
				data.stick_rudder_right = static_cast<u8>(std::clamp<long>(std::lroundf(value * 255.0f), 0, 255));
				UpdateStickRudder();
				break;

			case CID_STICK_HAT_L:
				data.stick_hat_left = static_cast<u8>(std::clamp<long>(std::lroundf(value * 255.0f), 0, 255));
				UpdateStickHatX();
				break;

			case CID_STICK_HAT_R:
				data.stick_hat_right = static_cast<u8>(std::clamp<long>(std::lroundf(value * 255.0f), 0, 255));
				UpdateStickHatX();
				break;

			case CID_STICK_HAT_U:
				data.stick_hat_up = static_cast<u8>(std::clamp<long>(std::lroundf(value * 255.0f), 0, 255));
				UpdateStickHatY();
				break;

			case CID_STICK_HAT_D:
				data.stick_hat_down = static_cast<u8>(std::clamp<long>(std::lroundf(value * 255.0f), 0, 255));
				UpdateStickHatY();
				break;

			default:
				break;
		}
	}

	void PadState::UpdateSteering()
	{
		u16 value;
		if (data.steering_left > 0)
			value = static_cast<u16>(std::max<int>(steering_range - data.steering_left, 0));
		else
			value = static_cast<u16>(std::min<int>(steering_range + data.steering_right, steering_range * 2));

		// TODO: Smoothing, don't jump too much
		//data.steering = value;
		if (value < data.steering)
			data.steering -= std::min<u16>(data.steering - value, steering_step);
		else if (value > data.steering)
			data.steering += std::min<u16>(value - data.steering, steering_step);
	}

	void PadState::UpdateHatSwitch()
	{
		if (data.hat_up && data.hat_right)
			data.hatswitch = 1;
		else if (data.hat_right && data.hat_down)
			data.hatswitch = 3;
		else if (data.hat_down && data.hat_left)
			data.hatswitch = 5;
		else if (data.hat_left && data.hat_up)
			data.hatswitch = 7;
		else if (data.hat_up)
			data.hatswitch = 0;
		else if (data.hat_right)
			data.hatswitch = 2;
		else if (data.hat_down)
			data.hatswitch = 4;
		else if (data.hat_left)
			data.hatswitch = 6;
		else
			data.hatswitch = 8;
	}

	void PadState::UpdateStickX()
	{
		if (data.stick_left > 0)
			data.stick_x = static_cast<u8>(std::max<int>(stick_range - data.stick_left, 0));
		else if (data.stick_right > 0)
			data.stick_x = static_cast<u8>(std::min<int>(stick_range + data.stick_right, stick_range * 2));
		else
			data.stick_x = 0x80;
	}

	void PadState::UpdateStickY()
	{
		if (data.stick_up > 0)
			data.stick_y = static_cast<u8>(std::max<int>(stick_range - data.stick_up, 0));
		else if (data.stick_down > 0)
			data.stick_y = static_cast<u8>(std::min<int>(stick_range + data.stick_down, stick_range * 2));
		else
			data.stick_y = 0x80;
	}

	void PadState::UpdateStickThrottle()
	{
		if (data.stick_throttle_up > 0)
			data.stick_throttle = static_cast<u8>(std::min<int>(stick_range + data.stick_throttle_up, stick_range * 2));
		else if (data.stick_throttle_down > 0)
			data.stick_throttle = static_cast<u8>(std::max<int>(stick_range - data.stick_throttle_down, 0));
		else
			data.stick_throttle = 0x80;
	}

	void PadState::UpdateStickRudder()
	{
		if (data.stick_rudder_left > 0)
			data.stick_rudder = static_cast<u8>(std::max<int>(stick_range - data.stick_rudder_left, 0));
		else if (data.stick_rudder_right > 0)
			data.stick_rudder = static_cast<u8>(std::min<int>(stick_range + data.stick_rudder_right, stick_range * 2));
		else
			data.stick_rudder = 0x80;
	}

	void PadState::UpdateStickHatX()
	{
		if (data.stick_hat_left > 0)
			data.stick_hat_x = static_cast<u8>(std::max<int>(stick_range - data.stick_hat_left, 0));
		else if (data.stick_hat_right > 0)
			data.stick_hat_x = static_cast<u8>(std::min<int>(stick_range + data.stick_hat_right, stick_range * 2));
		else
			data.stick_hat_x = 0x80;
	}

	void PadState::UpdateStickHatY()
	{
		if (data.stick_hat_up > 0)
			data.stick_hat_y = static_cast<u8>(std::max<int>(stick_range - data.stick_hat_up, 0));
		else if (data.stick_hat_down > 0)
			data.stick_hat_y = static_cast<u8>(std::min<int>(stick_range + data.stick_hat_down, stick_range * 2));
		else
			data.stick_hat_y = 0x80;
	}

	bool PadState::HasFF() const
	{
		// only do force feedback for wheels and flightforce...
		return (type <= WT_GT_FORCE || type == WT_FLIGHTSTICK_FLIGHTFORCE);
	}

	void PadState::OpenFFDevice()
	{
		if (mFFdevName.empty())
			return;

		mFFdev.reset();
		mFFdev = SDLFFDevice::Create(mFFdevName);
	}

	static void pad_handle_data(USBDevice* dev, USBPacket* p)
	{
		PadState* s = USB_CONTAINER_OF(dev, PadState, dev);

		switch (p->pid)
		{
			case USB_TOKEN_IN:
				if (p->ep->nr == 1)
				{
					int ret = s->TokenIn(p->buffer_ptr, p->buffer_size);

					if (ret > 0)
						p->actual_length += std::min<u32>(static_cast<u32>(ret), p->buffer_size);
					else
						p->status = ret;
				}
				else
				{
					goto fail;
				}
				break;
			case USB_TOKEN_OUT:
				/*Console.Warning("usb-pad: data token out len=0x%X %X,%X,%X,%X,%X,%X,%X,%X\n",len,
			data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7]);*/
				//Console.Warning("usb-pad: data token out len=0x%X\n",len);
				s->TokenOut(p->buffer_ptr, p->buffer_size);
				break;
			default:
			fail:
				p->status = USB_RET_STALL;
				break;
		}
	}

	static void pad_handle_reset(USBDevice* dev)
	{
		PadState* s = USB_CONTAINER_OF(dev, PadState, dev);
		s->Reset();
	}

	static void pad_handle_control(USBDevice* dev, USBPacket* p, int request, int value,
		int index, int length, uint8_t* data)
	{
		PadState* s = USB_CONTAINER_OF(dev, PadState, dev);
		int ret = 0;

		switch (request)
		{
			case DeviceRequest | USB_REQ_GET_DESCRIPTOR:
				ret = usb_desc_handle_control(dev, p, request, value, index, length, data);
				if (ret < 0)
					goto fail;

				break;
			case InterfaceRequest | USB_REQ_GET_DESCRIPTOR: //GT3
				switch (value >> 8)
				{
					// TODO: Move to constructor
					case USB_DT_REPORT:
						if (s->type == WT_DRIVING_FORCE_PRO || s->type == WT_DRIVING_FORCE_PRO_1102)
						{
							ret = sizeof(pad_driving_force_pro_hid_report_descriptor);
							memcpy(data, pad_driving_force_pro_hid_report_descriptor, ret);
						}
						else if (s->type == WT_GT_FORCE)
						{
							ret = sizeof(pad_gtforce_hid_report_descriptor);
							memcpy(data, pad_gtforce_hid_report_descriptor, ret);
						}
						else if (s->type == WT_KEYBOARDMANIA_CONTROLLER)
						{
							ret = sizeof(kbm_hid_report_descriptor);
							memcpy(data, kbm_hid_report_descriptor, ret);
						}
						//else if (s->type == WT_FLIGHTSTICK_FS1) //missing?
						//{
						//}
						//else if (s->type == WT_FLIGHTSTICK_FS2) //missing?
						//{
						//}
						else if (s->type == WT_FLIGHTSTICK_FLIGHTFORCE)
						{
							ret = sizeof(flightforce_hid_report_descriptor);
							memcpy(data, flightforce_hid_report_descriptor, ret);
						}
						else if (s->type == WT_GENERIC)
						{
							ret = sizeof(pad_driving_force_hid_separate_report_descriptor);
							memcpy(data, pad_driving_force_hid_separate_report_descriptor, ret);
						}
						p->actual_length = ret;
						break;
					default:
						goto fail;
				}
				break;

			/* hid specific requests */
			case SET_REPORT:
				// no idea, Rock Band 2 keeps spamming this
				if (length > 0)
				{
					/* 0x01: Num Lock LED
			 * 0x02: Caps Lock LED
			 * 0x04: Scroll Lock LED
			 * 0x08: Compose LED
			 * 0x10: Kana LED */
					p->actual_length = 0;
					//p->status = USB_RET_SUCCESS;
				}
				break;
			case SET_IDLE:
				break;

			case VendorDeviceRequest: // flightstick request 0x00:
				switch (s->type)
				{
					case WT_FLIGHTSTICK_FS1:
					case WT_FLIGHTSTICK_FS2:
						data[0] = static_cast<u8>(~(s->data.buttons & 0x0f |
													(s->data.hat_up    ? 1u << 4 : 0) |
													(s->data.hat_right ? 1u << 5 : 0) |
													(s->data.hat_down  ? 1u << 6 : 0) |
													(s->data.hat_left  ? 1u << 7 : 0)));

						if (s->type == WT_FLIGHTSTICK_FS1)
							data[0] |= static_cast<u8>(1u << 1); // button_d = 1

						data[1] = static_cast<u8>(~((s->data.buttons << 1) & 0x60));

						p->actual_length = 2;
						break;
					default:
						goto fail;
				}
				break;

			case VendorDeviceRequest | 0x01: // flightstick request 0x01:
				switch (s->type)
				{
					case WT_FLIGHTSTICK_FS1:
						data[0] = 0xff;
						data[1] = 0xff;
						p->actual_length = 2;
						break;
					case WT_FLIGHTSTICK_FS2:
					{
						data[0] = static_cast<u8>(~(0x80 | ((s->data.buttons >> 2) & 0x70)));
						data[1] = static_cast<u8>(~(((~s->mode) & 0x3) | ((s->data.buttons >> 6) & 0xf8)));
						p->actual_length = 2;
					}
						break;
					default:
						goto fail;
				}
				break;

			case VendorDeviceOutRequest | 0x0C: // flightstick2 rumble
				switch (s->type)
				{
					case WT_FLIGHTSTICK_FS1:
					case WT_FLIGHTSTICK_FS2:
						//rumble (only supported on FS2)
						if (index == 0 && length == 1 && s->type == WT_FLIGHTSTICK_FS2)
							InputManager::SetUSBVibrationIntensity(s->port, std::min(static_cast<float>(data[0]) * (1.0f / 255.0f), 1.0f), 0);
						p->actual_length = length;
						break;
					default:
						goto fail;
				}
				break;

			default:
				ret = usb_desc_handle_control(dev, p, request, value, index, length, data);
				if (ret >= 0)
				{
					return;
				}
			fail:
				p->status = USB_RET_STALL;
				break;
		}
	}

	static void pad_handle_destroy(USBDevice* dev)
	{
		PadState* s = USB_CONTAINER_OF(dev, PadState, dev);
		delete s;
	}

	static void pad_init(PadState* s)
	{
		s->dev.speed = USB_SPEED_FULL;
		s->dev.klass.handle_attach = usb_desc_attach;
		s->dev.klass.handle_reset = pad_handle_reset;
		s->dev.klass.handle_control = pad_handle_control;
		s->dev.klass.handle_data = pad_handle_data;
		s->dev.klass.unrealize = pad_handle_destroy;
		s->dev.klass.usb_desc = &s->desc;
		s->dev.klass.product_desc = nullptr;

		usb_desc_init(&s->dev);
		usb_ep_init(&s->dev);
		pad_handle_reset(&s->dev);
	}

	USBDevice* PadDevice::CreateDevice(SettingsInterface& si, u32 port, u32 subtype) const
	{
		if (subtype >= WT_COUNT)
			return nullptr;

		PadState* s = new PadState(port, static_cast<PS2WheelTypes>(subtype));
		s->desc.full = &s->desc_dev;
		s->desc.str = df_desc_strings;

		const uint8_t* dev_desc = df_dev_descriptor;
		int dev_desc_len = sizeof(df_dev_descriptor);
		const uint8_t* config_desc = df_config_descriptor;
		int config_desc_len = sizeof(df_config_descriptor);

		switch (s->type)
		{
			case WT_DRIVING_FORCE_PRO:
			{
				dev_desc = dfp_dev_descriptor;
				dev_desc_len = sizeof(dfp_dev_descriptor);
				config_desc = dfp_config_descriptor;
				config_desc_len = sizeof(dfp_config_descriptor);
				s->desc.str = dfp_desc_strings;
			}
			break;
			case WT_DRIVING_FORCE_PRO_1102:
			{
				dev_desc = dfp_dev_descriptor_1102;
				dev_desc_len = sizeof(dfp_dev_descriptor_1102);
				config_desc = dfp_config_descriptor;
				config_desc_len = sizeof(dfp_config_descriptor);
				s->desc.str = dfp_desc_strings;
			}
			break;
			case WT_GT_FORCE:
			{
				dev_desc = gtf_dev_descriptor;
				dev_desc_len = sizeof(gtf_dev_descriptor);
				config_desc = gtforce_config_descriptor; //TODO
				config_desc_len = sizeof(gtforce_config_descriptor);
				s->desc.str = gtf_desc_strings;
			}
			default:
				break;
		}

		if (usb_desc_parse_dev(dev_desc, dev_desc_len, s->desc, s->desc_dev) < 0)
			goto fail;
		if (usb_desc_parse_config(config_desc, config_desc_len, s->desc_dev) < 0)
			goto fail;

		s->UpdateSettings(si, TypeName());
		pad_init(s);

		return &s->dev;

	fail:
		pad_handle_destroy(&s->dev);
		return nullptr;
	}

	const char* PadDevice::Name() const
	{
		return TRANSLATE_NOOP("USB", "Wheel Device");
	}

	const char* PadDevice::TypeName() const
	{
		return "Pad";
	}

	const char* PadDevice::IconName() const
	{
		return ICON_PF_STEERING_WHEEL_ALT;
	}

	bool PadDevice::Freeze(USBDevice* dev, StateWrapper& sw) const
	{
		PadState* s = USB_CONTAINER_OF(dev, PadState, dev);

		if (!sw.DoMarker("PadDevice"))
			return false;

		sw.Do(&s->data.last_steering);
		sw.DoPOD(&s->mFFstate);
		return true;
	}

	void PadDevice::UpdateSettings(USBDevice* dev, SettingsInterface& si) const
	{
		USB_CONTAINER_OF(dev, PadState, dev)->UpdateSettings(si, TypeName());
	}

	float PadDevice::GetBindingValue(const USBDevice* dev, u32 bind_index) const
	{
		const PadState* s = USB_CONTAINER_OF(dev, const PadState, dev);
		return s->GetBindValue(bind_index);
	}

	void PadDevice::SetBindingValue(USBDevice* dev, u32 bind_index, float value) const
	{
		PadState* s = USB_CONTAINER_OF(dev, PadState, dev);
		s->SetBindValue(bind_index, value);
	}

	std::span<const char*> PadDevice::SubTypes() const
	{
		static const char* subtypes[] = {TRANSLATE_NOOP("USB", "Driving Force"),
			TRANSLATE_NOOP("USB", "Driving Force Pro"), TRANSLATE_NOOP("USB", "Driving Force Pro (rev11.02)"),
			TRANSLATE_NOOP("USB", "GT Force")};
		return subtypes;
	}

	std::span<const InputBindingInfo> PadDevice::Bindings(u32 subtype) const
	{
		return GetWheelBindings(static_cast<PS2WheelTypes>(subtype));
	}

	std::span<const SettingInfo> PadDevice::Settings(u32 subtype) const
	{
		return GetWheelSettings(static_cast<PS2WheelTypes>(subtype));
	}

	void PadDevice::InputDeviceConnected(USBDevice* dev, const std::string_view identifier) const
	{
		PadState* s = USB_CONTAINER_OF(dev, PadState, dev);
		if (s->mFFdevName == identifier && s->HasFF())
			s->OpenFFDevice();
	}

	void PadDevice::InputDeviceDisconnected(USBDevice* dev, const std::string_view identifier) const
	{
		PadState* s = USB_CONTAINER_OF(dev, PadState, dev);
		if (s->mFFdevName == identifier)
			s->mFFdev.reset();
	}

	// ---- Rock Band drum kit ----

	const char* RBDrumKitDevice::Name() const
	{
		return TRANSLATE_NOOP("USB", "Rock Band Drum Kit");
	}

	const char* RBDrumKitDevice::TypeName() const
	{
		return "RBDrumKit";
	}

	const char* RBDrumKitDevice::IconName() const
	{
		return ICON_FA_DRUM;
	}

	USBDevice* RBDrumKitDevice::CreateDevice(SettingsInterface& si, u32 port, u32 subtype) const
	{
		PadState* s = new PadState(port, WT_ROCKBAND1_DRUMKIT);

		s->desc.full = &s->desc_dev;
		s->desc.str = rb1_desc_strings;

		if (usb_desc_parse_dev(rb1_dev_descriptor, sizeof(rb1_dev_descriptor), s->desc, s->desc_dev) < 0)
			goto fail;
		if (usb_desc_parse_config(rb1_config_descriptor, sizeof(rb1_config_descriptor), s->desc_dev) < 0)
			goto fail;

		pad_init(s);

		return &s->dev;

	fail:
		pad_handle_destroy(&s->dev);
		return nullptr;
	}

	std::span<const char*> RBDrumKitDevice::SubTypes() const
	{
		return {};
	}

	std::span<const InputBindingInfo> RBDrumKitDevice::Bindings(u32 subtype) const
	{
		static constexpr const InputBindingInfo bindings[] = {
			{"Blue", TRANSLATE_NOOP("USB", "Blue"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON0, GenericInputBinding::R1},
			{"Green", TRANSLATE_NOOP("USB", "Green"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON1, GenericInputBinding::Triangle},
			{"Red", TRANSLATE_NOOP("USB", "Red"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON2, GenericInputBinding::Circle},
			{"Yellow", TRANSLATE_NOOP("USB", "Yellow"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON3, GenericInputBinding::Square},
			{"Orange", TRANSLATE_NOOP("USB", "Orange"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON4, GenericInputBinding::Cross},
			{"Select", TRANSLATE_NOOP("USB", "Select"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON8, GenericInputBinding::Select},
			{"Start", TRANSLATE_NOOP("USB", "Start"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON9, GenericInputBinding::Start},
			{"D-Pad Up", TRANSLATE_NOOP("USB", "D-Pad Up"), nullptr, InputBindingInfo::Type::Button, CID_DPAD_UP, GenericInputBinding::DPadUp},
			{"D-Pad Right", TRANSLATE_NOOP("USB", "D-Pad Right"), nullptr, InputBindingInfo::Type::Button, CID_DPAD_RIGHT, GenericInputBinding::DPadRight},
			{"D-Pad Down", TRANSLATE_NOOP("USB", "D-Pad Down"), nullptr, InputBindingInfo::Type::Button, CID_DPAD_DOWN, GenericInputBinding::DPadDown},
			{"D-Pad Left", TRANSLATE_NOOP("USB", "D-Pad Left"), nullptr, InputBindingInfo::Type::Button, CID_DPAD_LEFT, GenericInputBinding::DPadLeft},
		};

		return bindings;
	}

	std::span<const SettingInfo> RBDrumKitDevice::Settings(u32 subtype) const
	{
		return {};
	}

	// ---- Keyboardmania ----

	const char* KeyboardmaniaDevice::Name() const
	{
		return TRANSLATE_NOOP("USB", "KeyboardMania");
	}

	const char* KeyboardmaniaDevice::TypeName() const
	{
		return "Keyboardmania";
	}

	const char* KeyboardmaniaDevice::IconName() const
	{
		return ICON_PF_KEYBOARDMANIA;
	}

	std::span<const char*> KeyboardmaniaDevice::SubTypes() const
	{
		return {};
	}

	std::span<const InputBindingInfo> KeyboardmaniaDevice::Bindings(u32 subtype) const
	{
		static constexpr const InputBindingInfo bindings[] = {
			{"C1", TRANSLATE_NOOP("USB", "C 1"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON0, GenericInputBinding::Unknown},
			{"CSharp1", TRANSLATE_NOOP("USB", "C# 1"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON1, GenericInputBinding::Unknown},
			{"D1", TRANSLATE_NOOP("USB", "D 1"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON2, GenericInputBinding::Unknown},
			{"DSharp1", TRANSLATE_NOOP("USB", "D# 1"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON3, GenericInputBinding::Unknown},
			{"E1", TRANSLATE_NOOP("USB", "E 1"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON4, GenericInputBinding::Unknown},
			{"F1", TRANSLATE_NOOP("USB", "F 1"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON5, GenericInputBinding::Unknown},
			{"FSharp1", TRANSLATE_NOOP("USB", "F# 1"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON6, GenericInputBinding::Unknown},
			{"G1", TRANSLATE_NOOP("USB", "G 1"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON8, GenericInputBinding::Unknown},
			{"GSharp1", TRANSLATE_NOOP("USB", "G# 1"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON9, GenericInputBinding::Unknown},
			{"A1", TRANSLATE_NOOP("USB", "A 1"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON10, GenericInputBinding::Unknown},
			{"ASharp1", TRANSLATE_NOOP("USB", "A# 1"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON11, GenericInputBinding::Unknown},
			{"B1", TRANSLATE_NOOP("USB", "B 1"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON12, GenericInputBinding::Unknown},
			{"C2", TRANSLATE_NOOP("USB", "C 2"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON13, GenericInputBinding::Unknown},
			{"CSharp2", TRANSLATE_NOOP("USB", "C# 2"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON16, GenericInputBinding::Unknown},
			{"D2", TRANSLATE_NOOP("USB", "D 2"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON17, GenericInputBinding::Unknown},
			{"DSharp2", TRANSLATE_NOOP("USB", "D# 2"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON18, GenericInputBinding::Unknown},
			{"E2", TRANSLATE_NOOP("USB", "E 2"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON19, GenericInputBinding::Unknown},
			{"F2", TRANSLATE_NOOP("USB", "F 2"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON20, GenericInputBinding::Unknown},
			{"FSharp2", TRANSLATE_NOOP("USB", "F# 2"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON21, GenericInputBinding::Unknown},
			{"G2", TRANSLATE_NOOP("USB", "G 2"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON24, GenericInputBinding::Unknown},
			{"GSharp2", TRANSLATE_NOOP("USB", "G# 2"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON25, GenericInputBinding::Unknown},
			{"A2", TRANSLATE_NOOP("USB", "A 2"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON26, GenericInputBinding::Unknown},
			{"ASharp2", TRANSLATE_NOOP("USB", "A# 2"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON27, GenericInputBinding::Unknown},
			{"B2", TRANSLATE_NOOP("USB", "B 2"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON28, GenericInputBinding::Unknown},

			{"Start", TRANSLATE_NOOP("USB", "Start"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON22, GenericInputBinding::Unknown},
			{"Select", TRANSLATE_NOOP("USB", "Select"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON14, GenericInputBinding::Unknown},
			{"WheelUp", TRANSLATE_NOOP("USB", "Wheel Up"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON29, GenericInputBinding::Unknown},
			{"WheelDown", TRANSLATE_NOOP("USB", "Wheel Down"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON30, GenericInputBinding::Unknown},
		};

		return bindings;
	}

	std::span<const SettingInfo> KeyboardmaniaDevice::Settings(u32 subtype) const
	{
		return {};
	}

	USBDevice* KeyboardmaniaDevice::CreateDevice(SettingsInterface& si, u32 port, u32 subtype) const
	{
		PadState* s = new PadState(port, WT_KEYBOARDMANIA_CONTROLLER);

		s->desc.full = &s->desc_dev;
		s->desc.str = kbm_desc_strings;

		if (usb_desc_parse_dev(kbm_dev_descriptor, sizeof(kbm_dev_descriptor), s->desc, s->desc_dev) < 0)
			goto fail;
		if (usb_desc_parse_config(kbm_config_descriptor, sizeof(kbm_config_descriptor), s->desc_dev) < 0)
			goto fail;

		pad_init(s);

		return &s->dev;

	fail:
		pad_handle_destroy(&s->dev);
		return nullptr;
	}

	// ---- Flightstick (FS1, FS2, FlightForce) ----

	const char* FlightStickDevice::Name() const
	{
		return TRANSLATE_NOOP("USB", "Flight Stick Device");
	}

	const char* FlightStickDevice::TypeName() const
	{
		return "FlightStick";
	}

	const char* FlightStickDevice::IconName() const
	{
		return ICON_PF_GAMEPAD;
	}

	std::span<const char*> FlightStickDevice::SubTypes() const
	{
		static const char* subtypes[] = {
			TRANSLATE_NOOP("USB", "HP2-13 (FS1)"),
			TRANSLATE_NOOP("USB", "HP2-217 (FS2)"),
			TRANSLATE_NOOP("USB", "Flight Force"),
		};
		return subtypes;
	}

	std::span<const InputBindingInfo> FlightStickDevice::Bindings(u32 subtype) const
	{
		//using macros for shared data
#define BINDINGS_FLIGHTSTICK_SHARED_ANALOG_STICK_THROTTLE_RUDDER \
		{"StickLeft", TRANSLATE_NOOP("USB", "Stick Left"), nullptr, InputBindingInfo::Type::HalfAxis, CID_STICK_L, GenericInputBinding::LeftStickLeft}, \
		{"StickRight", TRANSLATE_NOOP("USB", "Stick Right"), nullptr, InputBindingInfo::Type::HalfAxis, CID_STICK_R, GenericInputBinding::LeftStickRight}, \
		{"StickUp", TRANSLATE_NOOP("USB", "Stick Up"), nullptr, InputBindingInfo::Type::HalfAxis, CID_STICK_U, GenericInputBinding::LeftStickUp}, \
		{"StickDown", TRANSLATE_NOOP("USB", "Stick Down"), nullptr, InputBindingInfo::Type::HalfAxis, CID_STICK_D, GenericInputBinding::LeftStickDown}, \
		{"ThrottleUp", TRANSLATE_NOOP("USB", "Throttle Up"), nullptr, InputBindingInfo::Type::HalfAxis, CID_STICK_THROTTLE_U, GenericInputBinding::R2}, \
		{"ThrottleDown", TRANSLATE_NOOP("USB", "Throttle Down"), nullptr, InputBindingInfo::Type::HalfAxis, CID_STICK_THROTTLE_D, GenericInputBinding::L2}, \
		{"RudderLeft", TRANSLATE_NOOP("USB", "Rudder Left"), nullptr, InputBindingInfo::Type::HalfAxis, CID_STICK_RUDDER_L, GenericInputBinding::L1}, \
		{ "RudderRight", TRANSLATE_NOOP("USB", "Rudder Right"), nullptr, InputBindingInfo::Type::HalfAxis, CID_STICK_RUDDER_R, GenericInputBinding::R1 }

#define BINDINGS_FLIGHTSTICK_SHARED_ANALOG_HAT_BUTTONS \
		{"HatLeft", TRANSLATE_NOOP("USB", "Stick Hat Left"), nullptr, InputBindingInfo::Type::HalfAxis, CID_STICK_HAT_L, GenericInputBinding::RightStickLeft}, \
		{"HatRight", TRANSLATE_NOOP("USB", "Stick Hat Right"), nullptr, InputBindingInfo::Type::HalfAxis, CID_STICK_HAT_R, GenericInputBinding::RightStickRight}, \
		{"HatkUp", TRANSLATE_NOOP("USB", "Stick Hat Up"), nullptr, InputBindingInfo::Type::HalfAxis, CID_STICK_HAT_U, GenericInputBinding::RightStickUp}, \
		{"HatDown", TRANSLATE_NOOP("USB", "Stick Hat Down"), nullptr, InputBindingInfo::Type::HalfAxis, CID_STICK_HAT_D, GenericInputBinding::RightStickDown}, \
		{"TriangleA", TRANSLATE_NOOP("USB", "Triangle (A)"), nullptr, InputBindingInfo::Type::HalfAxis, CID_BRAKE, GenericInputBinding::Triangle}, \
		{"SquareB", TRANSLATE_NOOP("USB", "Square (B)"), nullptr, InputBindingInfo::Type::HalfAxis, CID_THROTTLE, GenericInputBinding::Square}

#define BINDINGS_FLIGHTSTICK_SHARED_DPAD \
		{"DPadUp", TRANSLATE_NOOP("USB", "D-Pad Up"), nullptr, InputBindingInfo::Type::Button, CID_DPAD_UP, GenericInputBinding::DPadUp}, \
		{"DPadDown", TRANSLATE_NOOP("USB", "D-Pad Down"), nullptr, InputBindingInfo::Type::Button, CID_DPAD_DOWN, GenericInputBinding::DPadDown}, \
		{"DPadLeft", TRANSLATE_NOOP("USB", "D-Pad Left"), nullptr, InputBindingInfo::Type::Button, CID_DPAD_LEFT, GenericInputBinding::DPadLeft}, \
		{"DPadRight", TRANSLATE_NOOP("USB", "D-Pad Right"), nullptr, InputBindingInfo::Type::Button, CID_DPAD_RIGHT, GenericInputBinding::DPadRight }

#define BINDINGS_FLIGHTSTICK_SHARED_BUTTONS \
		{"CrossTrigger", TRANSLATE_NOOP("USB", "Cross (Trigger)"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON5, GenericInputBinding::Cross}, \
		{"CircleLaunch", TRANSLATE_NOOP("USB", "Circle (Launch)"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON4, GenericInputBinding::Circle}, \
		{"Select", TRANSLATE_NOOP("USB", "Select (Fire C)"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON0, GenericInputBinding::Select}, \
		{"Start", TRANSLATE_NOOP("USB", "Start"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON3, GenericInputBinding::Start}, \
		{"HatClick", TRANSLATE_NOOP("USB", "Hat Click"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON2, GenericInputBinding::R3}

		const PS2WheelTypes flightstick_subtype = static_cast<PS2WheelTypes>(WT_FLIGHTSTICK_FS1 + subtype);

		switch (flightstick_subtype)
		{
			case WT_FLIGHTSTICK_FS1:
			{
				static constexpr const InputBindingInfo bindings_fs1[] = {
					BINDINGS_FLIGHTSTICK_SHARED_ANALOG_STICK_THROTTLE_RUDDER,
					BINDINGS_FLIGHTSTICK_SHARED_ANALOG_HAT_BUTTONS,
					BINDINGS_FLIGHTSTICK_SHARED_DPAD,
					BINDINGS_FLIGHTSTICK_SHARED_BUTTONS
				};
				return bindings_fs1;
			}
			case WT_FLIGHTSTICK_FS2:
			{
				static constexpr const InputBindingInfo bindings_fs2[] = {
					BINDINGS_FLIGHTSTICK_SHARED_ANALOG_STICK_THROTTLE_RUDDER,
					BINDINGS_FLIGHTSTICK_SHARED_ANALOG_HAT_BUTTONS,
					BINDINGS_FLIGHTSTICK_SHARED_DPAD,
					{"Dpad2Up", TRANSLATE_NOOP("USB", "D-Pad 2 Up"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON10, GenericInputBinding::Unknown},
					{"Dpad2Down", TRANSLATE_NOOP("USB", "D-Pad 2 Down"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON12, GenericInputBinding::Unknown},
					{"Dpad2Left", TRANSLATE_NOOP("USB", "D-Pad 2 Left"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON13, GenericInputBinding::Unknown},
					{"Dpad2Right", TRANSLATE_NOOP("USB", "D-Pad 2 Right"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON11, GenericInputBinding::Unknown},
					BINDINGS_FLIGHTSTICK_SHARED_BUTTONS,
					{"D", TRANSLATE_NOOP("USB", "D"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON1, GenericInputBinding::L3},
					{"SW1", TRANSLATE_NOOP("USB", "SW1 (Pinky Trigger)"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON9, GenericInputBinding::Unknown},
					{"Dpad3Left", TRANSLATE_NOOP("USB", "D-Pad 3 Left"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON8, GenericInputBinding::Unknown},
					{"Dpad3Middle", TRANSLATE_NOOP("USB", "D-Pad 3 Middle"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON7, GenericInputBinding::Unknown},
					{"Dpad3Right", TRANSLATE_NOOP("USB", "D-Pad 3 Right"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON6, GenericInputBinding::Unknown},
					{"Motor", TRANSLATE_NOOP("USB", "Motor"), nullptr, InputBindingInfo::Type::Motor, 0, GenericInputBinding::LargeMotor},
				};
				return bindings_fs2;
			}
			case WT_FLIGHTSTICK_FLIGHTFORCE:
			{
				static constexpr const InputBindingInfo bindings_ff[] = {
					BINDINGS_FLIGHTSTICK_SHARED_ANALOG_STICK_THROTTLE_RUDDER,
					BINDINGS_FLIGHTSTICK_SHARED_DPAD,
					{"B1", TRANSLATE_NOOP("USB", "B1 (Trigger)"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON0, GenericInputBinding::Cross},
					{"B2", TRANSLATE_NOOP("USB", "B2"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON1, GenericInputBinding::Square},
					{"B3", TRANSLATE_NOOP("USB", "B2"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON2, GenericInputBinding::Circle},
					{"B4", TRANSLATE_NOOP("USB", "B4"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON3, GenericInputBinding::Triangle},
					{"B5", TRANSLATE_NOOP("USB", "B5"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON4, GenericInputBinding::R3},
					{"B6", TRANSLATE_NOOP("USB", "B6"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON5, GenericInputBinding::Start},
					{"B7", TRANSLATE_NOOP("USB", "B7"), nullptr, InputBindingInfo::Type::Button, CID_BUTTON6, GenericInputBinding::Select},
					{"FFDevice", TRANSLATE_NOOP("USB", "Force Feedback"), nullptr, InputBindingInfo::Type::Device, 0, GenericInputBinding::Unknown},
				};
				return bindings_ff;
			}
			default:
				break;
		}
		return {};
#undef BINDINGS_FLIGHTSTICK_SHARED_ANALOG_STICK_THROTTLE_RUDDER
#undef BINDINGS_FLIGHTSTICK_SHARED_ANALOG_HAT_BUTTONS
#undef BINDINGS_FLIGHTSTICK_SHARED_DPAD
#undef BINDINGS_FLIGHTSTICK_SHARED_BUTTONS
	}

	void FlightStickDevice::UpdateSettings(USBDevice* dev, SettingsInterface& si) const
	{
		//FlightStickDeviceState* s = USB_CONTAINER_OF(dev, FlightStickDeviceState, dev);
		PadState* s = USB_CONTAINER_OF(dev, PadState, dev);
		s->mode = USB::GetConfigInt(si, s->port, TypeName(), "Mode", 3);
		if (s->type == WT_FLIGHTSTICK_FS2)
		{
			Host::AddKeyedOSDMessage("Pad", fmt::format("FlightStick Mode: {}", s->mode), Host::OSD_QUICK_DURATION);
		}
	}

	std::span<const SettingInfo> FlightStickDevice::Settings(u32 subtype) const
	{
		const PS2WheelTypes flightstick_subtype = static_cast<PS2WheelTypes>(WT_FLIGHTSTICK_FS1 + subtype);

		static const char* s_mode_options[] = {
			TRANSLATE_NOOP("Pad", "1"),
			TRANSLATE_NOOP("Pad", "2"),
			TRANSLATE_NOOP("Pad", "3"),
			nullptr
		};

		static constexpr const SettingInfo mode = {
			SettingInfo::Type::IntegerList, // type
			"Mode", // name
			TRANSLATE_NOOP("Pad", "Mode switch"), // display name
			TRANSLATE_NOOP("Pad", "Set the stick mode switch position"), // description
			"3", // default value
			"1", // min value
			"3", // max value
			nullptr, // step value
			nullptr, // format
			s_mode_options, // options for integer lists
			nullptr, // options for string lists
			0.0f // multiplier
		};

		static constexpr const SettingInfo info[] = {mode};

		if (flightstick_subtype == WT_FLIGHTSTICK_FS2)
			return info;
		else
			return {};
	}

	USBDevice* FlightStickDevice::CreateDevice(SettingsInterface& si, u32 port, u32 subtype) const
	{
		const PS2WheelTypes flightstick_subtype = static_cast<PS2WheelTypes>(WT_FLIGHTSTICK_FS1 + subtype);
		PadState* s = new PadState(port, flightstick_subtype);

		s->desc.full = &s->desc_dev;
		s->desc.str = flightforce_desc_strings;

		const uint8_t* dev_desc = flightforce_dev_descriptor;
		int dev_desc_len = sizeof(flightforce_dev_descriptor);
		const uint8_t* config_desc = flightforce_config_descriptor;
		int config_desc_len = sizeof(flightforce_config_descriptor);

		switch (s->type)
		{
			case WT_FLIGHTSTICK_FS1:
			{
				dev_desc = fst01_dev_descriptor;
				dev_desc_len = sizeof(fst01_dev_descriptor);
				config_desc = flightstick_config_descriptor;
				config_desc_len = sizeof(flightstick_config_descriptor);
				s->desc.str = flightstick_desc_strings;
			}
			break;
			case WT_FLIGHTSTICK_FS2:
			{
				dev_desc = fst02_dev_descriptor;
				dev_desc_len = sizeof(fst02_dev_descriptor);
				config_desc = flightstick_config_descriptor;
				config_desc_len = sizeof(flightstick_config_descriptor);
				s->desc.str = flightstick_desc_strings;
			}
			break;
			default:
				break;
		}

		if (usb_desc_parse_dev(dev_desc, dev_desc_len, s->desc, s->desc_dev) < 0)
			goto fail;
		if (usb_desc_parse_config(config_desc, config_desc_len, s->desc_dev) < 0)
			goto fail;

		s->UpdateSettings(si, TypeName());
		pad_init(s);

		return &s->dev;

	fail:
		pad_handle_destroy(&s->dev);
		return nullptr;
	}
} // namespace usb_pad

#include <cassert>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <string_view>

#include "ArgParser.hpp"


struct Status {
	struct Error {
		bool noMedia : 1;
		bool endOfMedia : 1;
		bool cutterJam : 1;
		bool weakBatteries : 1;
		bool printerInUse : 1;
		bool notUsed1 : 1;
		bool highVoltageAdapter : 1;
		bool notUsed2 : 1;
		bool replaceMedia : 1;
		bool expansionBuffer : 1;
		bool communication : 1;
		bool communicationBufferFull : 1;
		bool coverOpen : 1;
		bool overheating : 1;
		bool blackMarkingNotDetected : 1;
		bool systemError : 1;
	};

	struct ExtendedError {
		enum Value : uint8_t {
			FleTapeEnd = 0x10,
			HighResolutionOrDraftPrintingError = 0x1d,
			AdapterPullOrInsertError = 0x1e,
			IncompatibleMediaError = 0x21,
		};
	};

	struct StatusType {
		enum Value : uint8_t {
			ReplyToStatusRequest = 0x00,
			PrintingCompleted = 0x01,
			ErrorOccurred = 0x02,
			ExitIFMode = 0x03,
			TurnedOff = 0x04,
			Notification = 0x05,
			PhaseChange = 0x06,
		};
	};

	struct NotificationNumber {
		enum Value : uint8_t {
			NotAvailable = 0x00,
			CoverOpen = 0x01,
			CoverClosed = 0x02,
			CoolingStarted = 0x03,
			CoolingFinished = 0x04,
		};
	};

	struct TapeColourInformation {
		enum Value : uint8_t {
			White = 0x01,
			Other = 0x02,
			Clear = 0x03,
			Red = 0x04,
			Blue = 0x05,
			Yellow = 0x06,
			Green = 0x07,
			Black = 0x08,
			ClearWhiteText = 0x09,
			MatteWhite = 0x20,
			MatteClear = 0x21,
			MatteSilver = 0x22,
			SatinGold = 0x23,
			SatinSilver = 0x24,
			BlueD = 0x30,
			RedD = 0x31,
			FluorescentOrange = 0x40,
			FluorescentYellow = 0x41,
			BerryPinkS = 0x50,
			LightGrayS = 0x51,
			YellowF = 0x60,
			PinkF = 0x61,
			BlueF = 0x62,
			WhiteHSTube = 0x70,
			WhiteFlexID = 0x90,
			YellowFlexID = 0x91,
			Cleaning = 0xf0,
			Stencil = 0xf1,
			Incompatible = 0xff,
		};
	};

	struct TextColourInformation {
		enum Value : uint8_t {
			White = 0x01,
			Other = 0x02,
			Red = 0x04,
			Blue = 0x05,
			Black = 0x08,
			Gold = 0x0a,
			BlueF = 0x62,
			Cleaning = 0xf0,
			Stencil = 0xf1,
			Incompatible = 0xff,
		};
	};

	struct BatteryLevel {
		enum Value : uint8_t {
			Full = 0x00,
			Half = 0x01,
			Low = 0x02,
			NeedToBeCharged = 0x03,
			UsingAcAdapter = 0x04,
			Unknown = 0xff,
		};
	};

	uint8_t printHeadMark;
	uint8_t size;
	uint8_t brotherCode, seriesCode, modelCode, countryCode;
	BatteryLevel::Value batteryLevel;
	ExtendedError::Value extendedError;
	Error error;
	uint8_t mediaWidth, mediaType;
	uint8_t numberOfColours;
	uint8_t fonts, japaneseFonts;
	uint8_t mode;
	uint8_t density;
	uint8_t mediaLength;
	StatusType::Value statusType;
	uint8_t phaseType, phaseNumber0, phaseNumber1;
	NotificationNumber::Value notificationNumber;
	uint8_t expansionArea;
	TapeColourInformation::Value tapeColourInformation;
	TextColourInformation::Value textColourInformation;
	uint8_t reserved[6];

	std::string_view modelCodeStr() const
	{
		switch (modelCode) {
			case 0x71: return "PT-P900";
			case 0x69: return "PT-P900W";
			case 0x70: return "PT-P950NW";
			case 0x78: return "PT-P910BT";
			default: return "unrecognised";
		}
	}

	std::string errorStr() const
	{
		bool fleTapeEnd = extendedError == ExtendedError::FleTapeEnd;
		bool highResolutionOrDraftPrintingError = extendedError == ExtendedError::HighResolutionOrDraftPrintingError;
		bool adapterPullOrInsertError = extendedError == ExtendedError::AdapterPullOrInsertError;
		bool incompatibleMediaError = extendedError == ExtendedError::IncompatibleMediaError;

		bool first = true;
		std::string errMsg;
		for (const auto &[err, msg] : std::initializer_list<std::pair<bool, std::string_view> >{
			{error.noMedia, "no media"},
			{error.endOfMedia, "end of media"},
			{error.cutterJam, "cutter jam"},
			{error.weakBatteries, "weak batteries"},
			{error.printerInUse, "printer in use"},
			{error.highVoltageAdapter, "high voltage adapter"},
			{error.replaceMedia, "replace media"},
			{error.expansionBuffer, "expansion buffer"},
			{error.communication, "communication"},
			{error.communicationBufferFull, "communication buffer full"},
			{error.coverOpen, "cover open"},
			{error.overheating, "overheating"},
			{error.blackMarkingNotDetected, "black marking not detected"},
			{error.systemError, "system error"},
			{fleTapeEnd, "Fle tape end"},
			{highResolutionOrDraftPrintingError, "High-resolution/draft printing error"},
			{adapterPullOrInsertError, "Adapter pull/insert error"},
			{incompatibleMediaError, "Incompatible media error"},
		}) {
			if (err) {
				if (!first)
					errMsg += ", ";
				errMsg += msg;
				first = false;
			}
		}

		return errMsg;
	}

	std::string_view mediaWidthStr() const
	{
		switch (mediaWidth) {
			case 0x00: return "no tape";
			case 0x04: return "3.5 mm";
			case 0x06: return "6 mm / HS 5.8 mm";
			case 0x09: return "9 mm / HS 8.8 mm";
			case 0x0c: return "12 mm / HS 11.7 mm";
			case 0x12: return "18 mm / HS 17.7 mm";
			case 0x18: return "24 mm / HS 23.6 mm";
			case 0x24: return "36 mm";
			case 0x15: return "FLe 21 mm x 45 mm";
			default: return "unrecognised tape width: " + std::to_string(static_cast<unsigned>(mediaWidth));
		}
	}

	std::string mediaTypeStr() const
	{
		switch (mediaType) {
			case 0x00: return "no media";
			case 0x01: return "laminated tape";
			case 0x03: return "non-laminated tape";
			case 0x04: return "fabric tape";
			case 0x11: return "heat-shrink tube";
			case 0x13: return "Fle tape";
			case 0x14: return "Flexible ID table";
			case 0x15: return "Satin tape";
			case 0x17: return "Heat-Shrink Tube (HS 3:1)";
			case 0xff: return "incompatible tape";
			default: return "unrecognised tape type: " + std::to_string(static_cast<unsigned>(mediaType));
		}
	}

	std::string_view statusStr() const
	{
		switch (statusType) {
			case StatusType::ReplyToStatusRequest: return "reply to status request";
			case StatusType::PrintingCompleted: return "printing completed";
			case StatusType::ErrorOccurred: return "error occurred";
			case StatusType::TurnedOff: return "turned off";
			case StatusType::Notification: return "notification";
			case StatusType::PhaseChange: return "phase changed";
			default:
				if (static_cast<uint8_t>(statusType) < 0x21)
					return "(not used)";
				return "(reserved)";
		}
	}

	std::string_view phaseStr() const
	{
		if (phaseType == 0x00) {
			switch (phaseNumber1) {
				case 0x00: return "editing state";
				case 0x01: return "feed";
				default: return "unrecognised editing state";
			}
		} else if (phaseType == 0x01) {
			switch (phaseNumber1) {
				case 0x00: return "printing";
				case 0x0a: return "(not used)";
				case 0x14: return "cover open while receiving";
				case 0x19: return "(not used)";
				default: return "unrecognised printing state";
			}
		}
		return "unrecognised phase type";
	}

	std::string_view notificationStr() const
	{
		switch (notificationNumber) {
			case NotificationNumber::NotAvailable: return "not available";
			case NotificationNumber::CoverOpen: return "cover open";
			case NotificationNumber::CoverClosed: return "cover closed";
			case NotificationNumber::CoolingStarted: return "cooling (started)";
			case NotificationNumber::CoolingFinished: return "cooling (finished)";
			default: return "unrecognised";
		}
	}

	std::string tapeColourStr() const
	{
		switch (tapeColourInformation) {
			case TapeColourInformation::White: return "white";
			case TapeColourInformation::Other: return "other";
			case TapeColourInformation::Clear: return "clear";
			case TapeColourInformation::Red: return "red";
			case TapeColourInformation::Blue: return "blue";
			case TapeColourInformation::Yellow: return "yellow";
			case TapeColourInformation::Green: return "green";
			case TapeColourInformation::Black: return "black";
			case TapeColourInformation::ClearWhiteText: return "clear white text";
			case TapeColourInformation::MatteWhite: return "matte white";
			case TapeColourInformation::MatteClear: return "matte clear";
			case TapeColourInformation::MatteSilver: return "matte silver";
			case TapeColourInformation::SatinGold: return "satin gold";
			case TapeColourInformation::SatinSilver: return "satin silver";
			case TapeColourInformation::BlueD: return "blue (D)";
			case TapeColourInformation::RedD: return "red (D)";
			case TapeColourInformation::FluorescentOrange: return "fluorescent orange";
			case TapeColourInformation::FluorescentYellow: return "fluorescent yellow";
			case TapeColourInformation::BerryPinkS: return "berry pink (S)";
			case TapeColourInformation::LightGrayS: return "light gray (S)";
			case TapeColourInformation::YellowF: return "yellow (F)";
			case TapeColourInformation::PinkF: return "pink (F)";
			case TapeColourInformation::BlueF: return "blue (F)";
			case TapeColourInformation::WhiteHSTube: return "white (Heat-shrink Tube)";
			case TapeColourInformation::WhiteFlexID: return "white (Flex. ID)";
			case TapeColourInformation::YellowFlexID: return "yellow (Flex. ID)";
			case TapeColourInformation::Cleaning: return "cleaning";
			case TapeColourInformation::Stencil: return "stencil";
			case TapeColourInformation::Incompatible: return "incompatible";
			default: return "unrecognised: " + std::to_string(static_cast<unsigned>(tapeColourInformation));
		}
	}

	std::string textColourStr() const
	{
		switch (textColourInformation) {
			case TextColourInformation::White: return "white";
			case TextColourInformation::Other: return "other";
			case TextColourInformation::Red: return "red";
			case TextColourInformation::Blue: return "blue";
			case TextColourInformation::Black: return "black";
			case TextColourInformation::Gold: return "gold";
			case TextColourInformation::BlueF: return "blue (F)";
			case TextColourInformation::Cleaning: return "cleaning";
			case TextColourInformation::Stencil: return "stencil";
			case TextColourInformation::Incompatible: return "incompatible";
			default: return "unrecognised: " + std::to_string(static_cast<unsigned>(textColourInformation));
		}
	}

	std::string_view batteryLevelStr() const
	{
		switch (batteryLevel) {
			case BatteryLevel::Full: return "full";
			case BatteryLevel::Half: return "half";
			case BatteryLevel::Low: return "low";
			case BatteryLevel::NeedToBeCharged: return "needs to be charged";
			case BatteryLevel::UsingAcAdapter: return "using AC adapter";
			case BatteryLevel::Unknown: return "unknown";
			default: return "unrecognised";
		}
	}
};

void check(uint8_t received, uint8_t expected, std::string_view label)
{
	if (received != expected) {
		std::cerr
			<< label << ": "
			<< std::hex << std::setfill('0') << std::setw(2)
			<< "received: " << static_cast<uint32_t>(received) << ", expected: " << static_cast<uint32_t>(expected) << "\n";
	}
}

void printHex(uint8_t e, std::string_view label)
{
	std::cout << label << ": " << std::hex << static_cast<uint32_t>(e) << "\n";
}

int main(int argc, char **argv)
{
	ArgParser parser;
	parser.addPositionalArgument(Arg{"input"});

	parser.parse(argc - 1, argv + 1);
	if (!parser.isValid()) {
		std::cerr << parser.getErrorMsg() << "\n";
		return 1;
	}

	Status status;
	assert(sizeof(status) == 32u);
	{
		const std::string &inputFile = parser.value("input");
		std::ifstream is{inputFile, std::ios::binary | std::ios::in};
		if (!is) {
			std::cerr << "Failed to open '" << inputFile << "'\n";
			return 1;
		}

		is.read(reinterpret_cast<char *>(&status), sizeof(status));
		if (!is) {
			if (!is.gcount()) {
				std::cerr << "Empty file: " << inputFile << "\n";
				return 1;
			}
			std::cerr << "Invalid input: " << inputFile << "\n";
			return 1;
		}
	}

	check(status.printHeadMark, 0x80, "print head mark");
	check(status.size, 0x20, "size");
	check(status.brotherCode, 0x42, "brother code");
	check(status.seriesCode, 0x30, "series code");
	std::cout << "model code: " << status.modelCodeStr() << "\n";
	check(status.countryCode, 0x30, "country code");

	std::cout << "battery level: " << status.batteryLevelStr() << "\n";
	std::cout << "error information: " << status.errorStr() << "\n";
	std::cout << "media width: " << status.mediaWidthStr() << "\n";
	std::cout << "media type: " << status.mediaTypeStr() << "\n";

	check(status.numberOfColours, 0x00, "number of colours");
	check(status.fonts, 0x00, "fonts");
	check(status.japaneseFonts, 0x00, "japanese fonts");

	printHex(status.mode, "mode");

	check(status.density, 0x00, "density");

	if (status.mediaWidth != 0x15)
		check(status.mediaLength, 0x00, "media length");
	else
		check(status.mediaLength, 0x2d, "media length");

	std::cout << "status: " << status.statusStr() << "\n";
	std::cout << "phase: " << status.phaseStr() << "\n";
	std::cout << "notification: " << status.notificationStr() << "\n";

	check(status.expansionArea, 0x00, "expansion area");

	std::cout << "tape colour: " << status.tapeColourStr() << "\n";
	std::cout << "text colour: " << status.textColourStr() << "\n";

	return 0;
}

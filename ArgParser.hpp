#pragma once

#include <iostream>
#include <format>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

class Arg {
public:
	Arg() = default;
	Arg(const Arg &) = default;
	Arg(std::string_view name) : m_name(name) {}

	bool isRequired() const
	{
		return m_required;
	}

	Arg & setRequired(bool required)
	{
		m_required = required;
		return *this;
	}

	bool isPresent() const
	{
		return m_present;
	}

	Arg & setPresent()
	{
		m_present = true;
		return *this;
	}

	Arg & setCount(unsigned cnt)
	{
		m_cnt = cnt;
		return *this;
	}

	unsigned count() const
	{
		return m_cnt;
	}

	std::string_view name() const
	{
		return m_name;
	}

	const std::string & value() const
	{
		return m_values[0];
	}

	const std::vector<std::string> & values() const
	{
		return m_values;
	}

	std::vector<std::string> & valuesRef()
	{
		return m_values;
	}

private:
	std::string_view m_name;
	bool m_required = true;
	bool m_present = false;
	unsigned m_cnt = 1;
	std::vector<std::string> m_values;
};


class ArgParser {
public:
	void parse(int argc, char **argv)
	{
		for (int i = 0; i < argc; ++i) {
			std::string str{argv[i]};
			if (str[0] == str[str.size() - 1] && (str[0] == '\'' || str[0] == '"'))
				str = str.substr(1, str.size() - 2);

			auto it = m_args.find(str);
			auto &arg = it->second;

			if (it == m_args.end()) {
				m_error = std::format("Unrecognised argument: '{}'", str);
				return;
			}
			if (static_cast<int>(i + arg.count()) >= argc) {
				m_error = std::format("Expected {} arguments after: '{}'", arg.count(), str);
				return;
			}

			arg.valuesRef().resize(arg.count());
			for (auto &v : arg.valuesRef()) {
				v = argv[++i];
				if (v[0] == v[v.size() - 1] && (v[0] == '\'' || v[0] == '"'))
					v = v.substr(1, v.size() - 2);
			}

			arg.setPresent();
		}

		for (auto &[key, arg] : m_args) {
			if (arg.isRequired() && !arg.isPresent()) {
				m_error = std::format("Missing argument: '{}'", key);
				return;
			}
		}
	}

	void addArgument(const Arg &arg)
	{
		m_args[arg.name()] = arg;
	}

	bool isValid() const
	{
		return m_error.empty();
	}

	const std::string &getErrorMsg() const
	{
		return m_error;
	}

	bool has(const std::string &name) const
	{
		return m_args.at(name).isPresent();
	}

	const std::string & value(std::string_view name) const
	{
		return m_args.at(name).value();
	}

	const std::vector<std::string> & values(std::string_view name) const
	{
		return m_args.at(name).values();
	}

private:
	std::unordered_map<std::string_view, Arg> m_args;
	mutable std::string m_error;
};

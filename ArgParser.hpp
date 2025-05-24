#pragma once

#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>

class Arg {
public:
	Arg() = default;

	Arg(std::string_view name) : m_name(name)
	{}

	//Arg & setCnt(unsigned cnt)
	//{
	//	m_cnt = cnt;
	//	return *this;
	//}

	std::string_view getName() const
	{
		return m_name;
	}

	bool isRequired() const
	{
		return m_required;
	}

private:
	std::string_view m_name;
	bool m_required = true;
	//unsigned m_cnt = 1;
};


class ArgParser {
public:
	void parse(int argc, char **argv)
	{
		for (int i = 0; i < argc; ++i) {
			std::string arg{argv[i]};
			auto it = m_args.find(arg);

			if (it == m_args.end()) {
				m_error = std::string{"Unrecognised argument: "} + "'" + arg + "'";
				return;
			}
			if (i == argc - 1) {
				m_error = std::string{"Expected argument after: "} + "'" + arg + "'";
				return;
			}

			m_values[arg] = std::string{argv[++i]};
		}

		for (auto &[key, arg] : m_args) {
			if (arg.isRequired() && !has(key)) {
				m_error = std::string{"Missing argument: "} + "'" + key + "'";
				return;
			}
		}
	}

	void addArgument(const Arg &arg)
	{
		m_args[std::string{arg.getName()}] = arg;
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
		return m_values.find(name) != m_values.end();
	}

	std::string get(const std::string &name) const
	{
		return m_values.at(name);
	}

private:
	std::unordered_map<std::string, Arg> m_args;
	std::unordered_map<std::string, std::string> m_values;
	mutable std::string m_error;
};

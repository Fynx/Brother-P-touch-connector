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

	Arg & setRequired()
	{
		m_required = true;
		return *this;
	}

	Arg & setOptional()
	{
		m_required = false;
		return *this;
	}

	bool isRepeatable() const
	{
		return m_repeatable;
	}

	Arg & setRepeatable()
	{
		m_repeatable = true;
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
	bool m_repeatable = false;
	bool m_present = false;
	unsigned m_cnt = 1;
	std::vector<std::string> m_values;
};


namespace {
	std::string stripQuotes(std::string &&str)
	{
		if (str[0] == str[str.size() - 1] && (str[0] == '\'' || str[0] == '"'))
			return str.substr(1, str.size() - 2);
		return str;
	}
}


class ArgParser {
public:
	void parse(int argc, char **argv)
	{
		for (int i = 0; i < argc; ++i) {
			std::string str = stripQuotes({argv[i]});

			if (i < static_cast<int>(m_posArgs.size())) {
				auto it = m_args.find(m_posArgs[i]);
				auto &arg = it->second;

				assert(arg.count() == 1);
				arg.valuesRef().resize(1);
				arg.valuesRef()[0] = str;

				arg.setPresent();
				continue;
			}

			auto it = m_args.find(str);
			if (it == m_args.end()) {
				m_error = std::format("Unrecognised argument: '{}'", str);
				return;
			}
			auto &arg = it->second;

			if (static_cast<int>(i + arg.count()) >= argc) {
				m_error = std::format("Expected {} arguments after: '{}'", arg.count(), str);
				return;
			}

			if (arg.isRepeatable()) {
				assert(arg.count() == 1);
				arg.valuesRef().push_back(stripQuotes(std::string{argv[++i]}));
			} else {
				arg.valuesRef().resize(arg.count());
				for (auto &v : arg.valuesRef())
					v = stripQuotes({argv[++i]});
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

	void addPositionalArgument(const Arg &arg)
	{
		addArgument(arg);
		m_posArgs.push_back(arg.name());
	}

	void addArgument(const Arg &arg)
	{
		assert(!m_args.contains(arg.name()));
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
	std::vector<std::string_view> m_posArgs;
	std::unordered_map<std::string_view, Arg> m_args;
	mutable std::string m_error;
};

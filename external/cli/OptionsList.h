#pragma once
//
// Project includes
//
#include "Option.h"

//
// Libs include
//

//
// STDLib/OS includes
//
#include <vector>
#include <string>
#include <map>

///////////////////////////////////////////////////////////////////////////////


class COptionsList
{
public:
    COptionsList() { m_AllOptions.reserve(20); }

    /**
     * @brief Add a program option to the list.
     * @param option The option.
     */
    bool AddOption(COption& option);

    /**
     * @brief Add a program option to the list.
     * If option with the same name (either shortname or longname)
     * is already exists then it is updated with new values.
     * 
     * @param shortName Short name of the option (ex: -v).
     * @param longName Long name of the option (ex: --version).
     * @param description Option description.
     * @param numArgs MAximum number of arguments of the option.
     * @param isRequired True if is required, false otherwise.
     */
    bool AddOption(const cli_string &shortName, const cli_string &longName, const cli_string &description, uint16_t numArgs, bool isRequired = false);

    /**
     * @brief Checks if short and long names have correct values (at least one of them is not empty).
     * Leading and trailing spaces are removed before check for empty strings for short and long names.
     *
     * @param shortName The short name of the option.
     * @param shortName The long name of the option.
     * @return True if both names have correct values. False if both shortName and longName strings are empty.
     * 
     */
    bool VerifyOptionNames(cli_string shortName, cli_string longName);

    /**
     * @brief Get option by the short name.
     *
     * @param name The name of the option.
     * @return A pointer to the option if found, nullptr otherwise.
     */
    COption *GetOptionByShortName(const cli_string &name);

    /**
     * @brief Get option by the long name.
     *
     * @param name The name of the option.
     * @return A pointer to the option if found, nullptr otherwise.
     */
    COption *GetOptionByLongName(const cli_string &name);

    /**
     * @brief Get option by the name (either short or long).
     *
     * @param name The name of the option.
     * @return A pointer to the option if found, nullptr otherwise.
     */
    COption* GetOption(const cli_string& name);

    /**
     * @brief Easy way to get option by the name (either short or long).
     *
     * @param name The name of the option.
     * @return A pointer to the option if found, nullptr otherwise.
     */
    COption* operator[](const cli_string& name) { return GetOption(name); }

    /**
    * @brief Registers pair of options that are mutually exclusive. They both cannot appear in one command line.
    *
    * @param name1 The name of the first option.
    * @param name2 The name of the second option.
    */
    void MutuallyExclusive(const cli_string& name1, const cli_string& name2);

    /**
    * @brief Registers three options that are pairwise mutually exclusive. 
    * Neither two of three (or all three) cannot appear in one command line.
    *
    * @param name1 The name of the first option.
    * @param name2 The name of the second option.
    * @param name3 The name of the third option.
    */
    void MutuallyExclusive(const cli_string& name1, const cli_string& name2, const cli_string& name3);

    /**
     * @brief Retrieve all the options that have required arguments (at leas one such argument).
     * @return A copy to a vector of pointers to options.
     */
    vector_option_pt GetOptionsWithRequiredArguments();

    /**
     * @brief Retrieve all options that are required.
     * @return A copy to a vector of pointers to options.
     */
    vector_option_pt GetRequiredOptions();

    /**
     * @brief Retrieve all options.
     * @return A reference to a vector of options.
     */
    inline vector_option_t& GetAllOptions() { return m_AllOptions; }

    void ResetOptions() { for (auto& opt : m_AllOptions) opt.Reset(); }

    size_t Count() { return m_AllOptions.size(); }

private:
    /** @brief List of all options. */
    vector_option_t m_AllOptions;
};

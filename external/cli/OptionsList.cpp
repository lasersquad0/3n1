//
// Project includes
//
#include "OptionsList.h"

//
// Libs include
//

//
// STDLib/OS includes
//

///////////////////////////////////////////////////////////////////////////////

static void TrimStr(cli_string& str)
{
    // remove all leading and traling spaces.
    size_t strBegin = str.find_first_not_of(' ');
    size_t strEnd   = str.find_last_not_of(' ');
    str.erase(strEnd + 1, str.size() - strEnd);
    str.erase(0, strBegin);
}

bool COptionsList::VerifyOptionNames(cli_string shortName, cli_string longName)
{
    TrimStr(shortName);
    TrimStr(longName);

    if (shortName.empty()) // if shortName is empty then longName must be non-empty
        return !longName.empty();
    else
        return true;
}

// if option exists already it will be "overwritten" by new option
bool COptionsList::AddOption(COption& option)
{
    const cli_string& sn = option.GetShortName();
    const cli_string& ln = option.GetLongName();
    
    if (!VerifyOptionNames(sn, ln)) return false; // option has invalid short and/or long names (or has incomplete data)

    COption* opt;

    if(sn.empty())
        opt = GetOptionByLongName(ln);
    else 
        opt = GetOptionByShortName(sn);

    if (opt == nullptr)
        m_AllOptions.emplace_back(option); 
    else
        opt->Assign(option);

    return true;
}

bool COptionsList::AddOption(const cli_string &shortName, const cli_string &longName, const cli_string &description, uint16_t numArgs, bool isRequired)
{
    if (!VerifyOptionNames(shortName, longName)) // option has invalid short and/or long names (or has incomplete data)
        return false; 

    COption* opt;

    if (shortName.empty())
        opt = GetOptionByLongName(longName);
    else
        opt = GetOptionByShortName(shortName);

    if (opt == nullptr)
        m_AllOptions.emplace_back(shortName, longName, description, numArgs, isRequired);
    else
        opt->Assign(shortName, longName, description, numArgs, isRequired);

    //COption o(shortName, longName, description, numArgs, isRequired);
    return true; //AddOption(o);
}

vector_option_pt COptionsList::GetOptionsWithRequiredArguments()
{
    vector_option_pt ret;

    for (auto &it : m_AllOptions)
    {
        //if (it.HasArgs())
        if(it.GetNumRequiredArgs() > 0)
            ret.emplace_back(&it);
    }

    return ret;
}

vector_option_pt COptionsList::GetRequiredOptions()
{
    vector_option_pt ret;

    for (auto &it : m_AllOptions)
    {
        if (it.IsRequired())
            ret.emplace_back(&it);
    }

    return ret;
}

COption *COptionsList::GetOptionByShortName(const cli_string &name)
{
    for (auto &it : m_AllOptions)
    {
        if (it.GetShortName() == name)
            return &it;
    }

    return nullptr;
}

COption* COptionsList::GetOptionByLongName(const cli_string &name)
{
    for (auto &it : m_AllOptions)
    {
        if (it.GetLongName() == name)
            return &it;
    }

    return nullptr;
}

COption* COptionsList::GetOption(const cli_string& name)
{
    auto opt = GetOptionByShortName(name);
    if (opt == nullptr)
        return GetOptionByLongName(name);
    return opt;
}

void COptionsList::MutuallyExclusive(const cli_string& name1, const cli_string& name2)
{
    auto opt1 = GetOption(name1);
    auto opt2 = GetOption(name2);
    opt1->Excludes(opt2); // we do not need opt2->Excludes(opt1) here because first statement defines exclude in both directions
}

void COptionsList::MutuallyExclusive(const cli_string& name1, const cli_string& name2, const cli_string& name3)
{
    auto opt1 = GetOption(name1);
    auto opt2 = GetOption(name2);
    auto opt3 = GetOption(name3);

    opt1->Excludes(opt2); // we do not need opt2->Excludes(opt1) here because first statement defines exclude in both directions
    opt1->Excludes(opt3);
    opt2->Excludes(opt3);
}


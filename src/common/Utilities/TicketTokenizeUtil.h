#pragma once

#include <algorithm>
#include <cctype>
#include <regex>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

struct SimilarSearchInput
{
    std::string title;
    std::string description;
    std::string reportPlain;
};

struct SimilarSearchTokens
{
    std::string fulltextBooleanQuery;
    std::vector<std::string> strongTokens;
};

static std::string ToLowerAscii(std::string s)
{
    std::ranges::transform(s, s.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return s;
}

static std::string KeepAlnum(std::string s)
{
    std::erase_if(s, [](unsigned char c) { return !std::isalnum(c); });
    return s;
}

static bool HasAlpha(std::string_view s)
{
    return std::ranges::any_of(s, [](unsigned char c) { return std::isalpha(c); });
}

static bool HasDigit(std::string_view s)
{
    return std::ranges::any_of(s, [](unsigned char c) { return std::isdigit(c); });
}

static bool IsStopword(std::string_view w)
{
    static const std::unordered_set<std::string> stop{
        "der",    "die",    "das",  "und",   "oder", "mit",   "ohne",     "bei",     "von",    "zu",
        "im",     "in",     "am",   "an",    "auf",  "bitte", "dringend", "sofort",  "fehler", "problem",
        "defekt", "kaputt", "geht", "nicht", "kein", "keine", "meldung",  "service", "anlage", "maschine"};

    return stop.contains(std::string(w));
}

static std::vector<std::string> ExtractWords(std::string_view input)
{
    static const std::regex rx(R"([a-zA-Z0-9]+(?:[-_/][a-zA-Z0-9]+)*)");

    std::vector<std::string> out;
    const std::string s(input);

    for (auto it = std::sregex_iterator(s.begin(), s.end(), rx); it != std::sregex_iterator(); ++it)
        out.push_back((*it)[0].str());

    return out;
}

static SimilarSearchTokens BuildSimilarSearchTokens(const SimilarSearchInput& in)
{
    const std::string merged = ToLowerAscii(in.title + " " + in.description + " " + in.reportPlain);

    std::unordered_set<std::string> uniqWords;
    uniqWords.reserve(128);

    std::vector<std::string> strong;
    strong.reserve(16);

    for (auto w : ExtractWords(merged))
    {
        w = ToLowerAscii(std::move(w));
        w = KeepAlnum(std::move(w));

        if (w.size() < 3)
            continue;

        if (IsStopword(w))
            continue;

        if (!uniqWords.insert(w).second)
            continue;

        const bool isStrong = HasAlpha(w) && HasDigit(w);
        if (isStrong)
            strong.push_back(w);
    }

    std::ranges::sort(strong, [](const auto& a, const auto& b) { return a.size() > b.size(); });

    if (strong.size() > 8)
        strong.resize(8);

    std::vector<std::string> words;
    words.reserve(uniqWords.size());
    for (const auto& w : uniqWords)
        words.push_back(w);

    std::ranges::sort(words, [](const auto& a, const auto& b) { return a.size() > b.size(); });

    if (words.size() > 12)
        words.resize(12);

    std::string ft;
    ft.reserve(256);

    for (const auto& w : words)
    {
        ft += '+';
        ft += w;
        ft += '*';
        ft += ' ';
    }

    SimilarSearchTokens out;
    out.fulltextBooleanQuery = std::move(ft);
    out.strongTokens = std::move(strong);
    return out;
}

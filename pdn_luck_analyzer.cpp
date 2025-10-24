// pdn_luck_analyzer.cpp
// C++17 port of the Python PDN "Luck" Analyzer
// Fixed to avoid raw-string regex pitfalls on some compilers.

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <optional>
#include <regex>
#include <sstream>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

using std::cerr;
using std::cout;
using std::endl;
using std::int64_t;
using std::optional;
using std::size_t;
using std::string;
using std::vector;

// ------------------------------- Utilities ----------------------------------

static inline string trim(const string& s) {
    size_t a = 0, b = s.size();
    while (a < b && std::isspace(static_cast<unsigned char>(s[a]))) ++a;
    while (b > a && std::isspace(static_cast<unsigned char>(s[b - 1]))) --b;
    return s.substr(a, b - a);
}

static inline bool starts_with(const string& s, const string& p) {
    return s.size() >= p.size() && std::equal(p.begin(), p.end(), s.begin());
}

static inline bool iequals(const string& a, const string& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (std::tolower((unsigned char)a[i]) != std::tolower((unsigned char)b[i])) return false;
    }
    return true;
}

static vector<string> split(const string& s, char sep) {
    vector<string> out;
    std::stringstream ss(s);
    string tok;
    while (std::getline(ss, tok, sep)) out.push_back(tok);
    return out;
}

// ------------------------------- Data types ----------------------------------

struct Entry {
    // score: None if "only move"
    optional<int> score;
    bool forced_only{false};
};

struct Metrics {
    int n_scored = 0;
    int n_forced = 0;
    vector<int> scores;
    int min_eval = 0;
    int max_eval = 0;
    int final_eval = 0;
    double mean_eval = 0.0;
    double stdev_eval = 0.0;

    vector<bool> losing_flags;
    vector<bool> winning_flags;
    int num_losing = 0;
    double frac_losing = 0.0;
    int longest_losing_span = 0;

    int num_winning = 0;
    double frac_winning = 0.0;
    int longest_winning_span = 0;

    int num_severe = 0;
    double frac_severe = 0.0;
    int longest_severe_span = 0;

    int comeback_magnitude = 0;
    vector<int> deltas;
    int max_swing = 0;
    optional<int> max_swing_idx; // index in deltas
    int max_pos_swing = 0;
    int max_neg_swing = 0;
    std::map<int,int> swing_counts;

    int lead_changes = 0;

    int forced_total = 0;
    int forced_adjacent_losing = 0;
    int forced_in_losing_tunnel = 0;

    // error handling
    string error;
};

// ------------------------------ Parsing PDN ----------------------------------

static std::unordered_map<string,string> parse_headers(const string& pdn) {
    std::unordered_map<string,string> headers;
    // Escaped-string regex to avoid raw-string portability issues
    std::regex re("\\[(\\w+)\\s+\"([^\"]*)\"\\]");
    auto it_begin = std::sregex_iterator(pdn.begin(), pdn.end(), re);
    auto it_end   = std::sregex_iterator();
    for (auto it = it_begin; it != it_end; ++it) {
        headers[(*it)[1].str()] = (*it)[2].str();
    }
    return headers;
}

static vector<Entry> parse_entries(const string& pdn) {
    // Find {...} blobs
    vector<Entry> entries;
    std::regex re("\\{([^}]*)\\}");
    auto it_begin = std::sregex_iterator(pdn.begin(), pdn.end(), re);
    auto it_end   = std::sregex_iterator();

    for (auto it = it_begin; it != it_end; ++it) {
        string content = trim((*it)[1].str());
        string low = content;
        std::transform(low.begin(), low.end(), low.begin(), [](unsigned char c){ return std::tolower(c); });

        if (low.find("only move") != string::npos) {
            entries.push_back(Entry{std::nullopt, true});
            continue;
        }

        // Robust: split by spaces, scan from back, collect last two int-like tokens
        vector<string> toks = split(content, ' ');
        optional<int> score;
        int found = 0;
        int last_two[2] = {0,0}; // reversed order
        for (int i = (int)toks.size() - 1; i >= 0; --i) {
            try {
                int v = std::stoi(toks[i]);
                if (found < 2) last_two[found] = v;
                ++found;
                if (found == 2) break;
            } catch (...) {
                continue;
            }
        }
        if (found >= 1) {
            // Choose the one nearer to the move (earlier in text): last_two[1] if present, else last_two[0]
            score = (found >= 2) ? last_two[1] : last_two[0];
        }
        entries.push_back(Entry{score, false});
    }
    return entries;
}

// Split games by headers starting with [Event "..."]
static vector<string> split_games(const string& pdn_text) {
    vector<string> games;
    // Find positions where a line starts with [Event "
    vector<size_t> idx;
    // Include position 0 if it starts with [Event
    if (starts_with(pdn_text, "[Event ")) idx.push_back(0);
    // Also search for "\n[Event "
    size_t pos = 0;
    while ((pos = pdn_text.find("\n[Event ", pos)) != string::npos) {
        idx.push_back(pos + 1); // start at '['
        pos += 7;
    }
    if (idx.empty()) {
        // no headers: treat entire file as one "game"
        string t = trim(pdn_text);
        if (!t.empty()) games.push_back(t);
        return games;
    }
    // Cut segments between indices
    for (size_t i = 0; i < idx.size(); ++i) {
        size_t begin = idx[i];
        size_t end = (i + 1 < idx.size()) ? idx[i+1] : pdn_text.size();
        games.push_back(trim(pdn_text.substr(begin, end - begin)));
    }
    return games;
}

// --------------------------- Metrics & helpers -------------------------------

static std::pair<int, vector<int>> consecutive_spans(const vector<bool>& flags) {
    int longest = 0;
    vector<int> spans;
    int cur = 0;
    for (bool f : flags) {
        if (f) { ++cur; }
        else {
            if (cur > 0) {
                longest = std::max(longest, cur);
                spans.push_back(cur);
                cur = 0;
            }
        }
    }
    if (cur > 0) {
        longest = std::max(longest, cur);
        spans.push_back(cur);
    }
    return {longest, spans};
}

static inline int sgn(int x) { return (x < 0) ? -1 : ((x > 0) ? 1 : 0); }

static Metrics analyze(const vector<Entry>& entries, int tdraw, int tedge, int severe, const vector<int>& swing_thresholds) {
    (void)tdraw; // not used directly in metrics; kept for future extensions
    Metrics m;

    // Collect numeric scores
    for (const auto& e : entries) {
        if (e.score.has_value()) m.scores.push_back(*e.score);
        if (e.forced_only) m.n_forced++;
    }
    m.n_scored = (int)m.scores.size();

    if (m.n_scored == 0) {
        m.error = "No numeric scores found in PDN braces.";
        return m;
    }

    // Core aggregates
    m.min_eval = *std::min_element(m.scores.begin(), m.scores.end());
    m.max_eval = *std::max_element(m.scores.begin(), m.scores.end());
    m.final_eval = m.scores.back();

    {
        // Mean
        long long sum = 0;
        for (int s : m.scores) sum += s;
        m.mean_eval = (double)sum / (double)m.n_scored;
        // Population stdev
        if (m.n_scored > 1) {
            long double acc = 0.0L;
            for (int s : m.scores) {
                long double d = (long double)s - (long double)m.mean_eval;
                acc += d * d;
            }
            m.stdev_eval = std::sqrt((double)(acc / (long double)m.n_scored));
        } else {
            m.stdev_eval = 0.0;
        }
    }

    // Flags
    m.losing_flags.resize(m.n_scored);
    m.winning_flags.resize(m.n_scored);
    vector<bool> severe_flags(m.n_scored);

    for (int i = 0; i < m.n_scored; ++i) {
        int s = m.scores[i];
        m.losing_flags[i]  = (s <= -tedge);
        m.winning_flags[i] = (s >=  tedge);
        severe_flags[i]    = (s <= -severe);
    }

    m.num_losing = (int)std::count(m.losing_flags.begin(), m.losing_flags.end(), true);
    m.frac_losing = (double)m.num_losing / (double)m.n_scored;
    m.longest_losing_span = consecutive_spans(m.losing_flags).first;

    m.num_winning = (int)std::count(m.winning_flags.begin(), m.winning_flags.end(), true);
    m.frac_winning = (double)m.num_winning / (double)m.n_scored;
    m.longest_winning_span = consecutive_spans(m.winning_flags).first;

    m.num_severe = (int)std::count(severe_flags.begin(), severe_flags.end(), true);
    m.frac_severe = (double)m.num_severe / (double)m.n_scored;
    m.longest_severe_span = consecutive_spans(severe_flags).first;

    // Comeback magnitude
    m.comeback_magnitude = std::max(0, m.final_eval - m.min_eval);

    // Swings
    if (m.n_scored >= 2) {
        m.deltas.reserve(m.n_scored - 1);
        for (int i = 0; i + 1 < m.n_scored; ++i) m.deltas.push_back(m.scores[i+1] - m.scores[i]);
    }
    m.max_swing = 0;
    if (!m.deltas.empty()) {
        int max_abs = 0;
        int max_idx = 0;
        for (int i = 0; i < (int)m.deltas.size(); ++i) {
            int a = std::abs(m.deltas[i]);
            if (a > max_abs) { max_abs = a; max_idx = i; }
        }
        m.max_swing = max_abs;
        m.max_swing_idx = max_idx;
    }
    m.max_pos_swing = 0;
    m.max_neg_swing = 0;
    for (int d : m.deltas) {
        if (d > m.max_pos_swing) m.max_pos_swing = d;
        if (d < m.max_neg_swing) m.max_neg_swing = d;
    }
    for (int thr : swing_thresholds) {
        int cnt = 0;
        for (int d : m.deltas) if (std::abs(d) > thr) ++cnt;
        m.swing_counts[thr] = cnt;
    }

    // Lead changes
    m.lead_changes = 0;
    {   int prev_sign = 0; bool have_prev = false;
        for (int s : m.scores) {
            int cur = sgn(s);
            if (have_prev && (prev_sign == -1 || prev_sign == 1) &&
                (cur == -1 || cur == 1) && cur != prev_sign) {
                m.lead_changes++;
            }
            if (cur != 0) { prev_sign = cur; have_prev = true; }
        }
    }

    // Forced move context
    vector<int> idx_scored;
    idx_scored.reserve(entries.size());
    for (int i = 0; i < (int)entries.size(); ++i)
        if (entries[i].score.has_value()) idx_scored.push_back(i);

    std::unordered_map<int,int> idx_to_scored_pos;
    idx_to_scored_pos.reserve(idx_scored.size());
    for (int pos = 0; pos < (int)idx_scored.size(); ++pos)
        idx_to_scored_pos[idx_scored[pos]] = pos;

    m.forced_total = 0;
    m.forced_adjacent_losing = 0;
    m.forced_in_losing_tunnel = 0;

    for (int i = 0; i < (int)entries.size(); ++i) {
        if (!entries[i].forced_only) continue;
        m.forced_total++;

        optional<int> left_pos;
        optional<int> right_pos;

        for (int j = i - 1; j >= 0; --j) {
            if (entries[j].score.has_value()) {
                left_pos = idx_to_scored_pos[j];
                break;
            }
        }
        for (int j = i + 1; j < (int)entries.size(); ++j) {
            if (entries[j].score.has_value()) {
                right_pos = idx_to_scored_pos[j];
                break;
            }
        }

        bool left_losing = left_pos.has_value()  ? m.losing_flags[*left_pos]  : false;
        bool right_losing= right_pos.has_value() ? m.losing_flags[*right_pos] : false;

        if (left_losing || right_losing) m.forced_adjacent_losing++;
        if (left_losing && right_losing) m.forced_in_losing_tunnel++;
    }

    return m;
}

// --------------------------- RI/SI & labeling --------------------------------

static double compute_rescue_index(const Metrics& me, int /*tedge*/,
                                   int A=60, int B=8, int C=50,
                                   double w1=0.4, double w2=0.25,
                                   double w3=0.2, double w4=0.15) {
    double term1 = std::max(0, -me.min_eval) / (double)A;
    double term2 = me.frac_losing;
    double term3 = (B ? (me.longest_losing_span / (double)B) : 0.0);
    double term4 = (C ? (me.max_swing / (double)C) : 0.0);
    return w1*term1 + w2*term2 + w3*term3 + w4*term4;
}

static double compute_squander_index(const Metrics& me,
                                     int A=60, int B=8, int C=50,
                                     double w1=0.4, double w2=0.25,
                                     double w3=0.2, double w4=0.15) {
    double term1 = std::max(0, +me.max_eval) / (double)A;
    double term2 = me.frac_winning;
    double term3 = (B ? (me.longest_winning_span / (double)B) : 0.0);
    double term4 = (C ? (me.max_swing / (double)C) : 0.0);
    return w1*term1 + w2*term2 + w3*term3 + w4*term4;
}

static string band(double v, double lo, double hi) {
    if (std::isnan(v)) return "NA";
    if (v <= lo) return "Low";
    if (v >= hi) return "High";
    return "Medium";
}

static int run_length_from(const vector<bool>& flags, int start) {
    int r = 0;
    for (int k = start; k < (int)flags.size(); ++k) {
        if (flags[k]) ++r; else break;
    }
    return r;
}

static std::tuple<string,string,string,string>
label_game(const string& result,
           const Metrics& me,
           double hi, double lo,
           int blunder_threshold, int decisive_span,
           int tedge) {
    (void)tedge; // present for parity

    const vector<int>& deltas = me.deltas;
    const vector<bool>& losing_flags = me.losing_flags;
    const vector<bool>& winning_flags = me.winning_flags;
    int lead_changes = me.lead_changes;

    double ri = compute_rescue_index(me, tedge);
    double si = compute_squander_index(me);
    string ri_band = band(ri, lo, hi);
    string si_band = band(si, lo, hi);

    int max_neg = me.max_neg_swing;
    int max_pos = me.max_pos_swing;

    optional<int> neg_idx, pos_idx;
    for (int i = 0; i < (int)deltas.size(); ++i) {
        if (!neg_idx.has_value() && deltas[i] == max_neg && max_neg < 0) { neg_idx = i; }
        if (!pos_idx.has_value() && deltas[i] == max_pos && max_pos > 0) { pos_idx = i; }
        if (neg_idx.has_value() && pos_idx.has_value()) break;
    }

    string res = trim(result);

    // Decisive blunders
    if (neg_idx.has_value() && -max_neg >= blunder_threshold) {
        int losing_after = run_length_from(losing_flags, *neg_idx + 1);
        if (losing_after >= decisive_span && res != "1-1") {
            std::ostringstream rsn;
            rsn << "-" << std::abs(max_neg) << " swing at ply " << (*neg_idx + 1)
                << " -> " << losing_after << "-ply losing span";
            return {"BlunderLoss", rsn.str(), ri_band, si_band};
        }
    }
    if (pos_idx.has_value() && max_pos >= blunder_threshold) {
        int winning_after = run_length_from(winning_flags, *pos_idx + 1);
        if (winning_after >= decisive_span && res != "1-1") {
            std::ostringstream rsn;
            rsn << "+" << max_pos << " swing at ply " << (*pos_idx + 1)
                << " -> " << winning_after << "-ply winning span";
            return {"BlunderWin", rsn.str(), ri_band, si_band};
        }
    }

    // Chaotic
    if (ri_band == "High" && si_band == "High" &&
        me.longest_losing_span >= decisive_span &&
        me.longest_winning_span >= decisive_span &&
        lead_changes >= 2) {
        if (res == "1-1") {
            return {"ChaoticDraw", "Alternating winning/losing spans with multiple lead changes", ri_band, si_band};
        } else {
            return {"ChaoticGame", "Alternating winning/losing spans with multiple lead changes", ri_band, si_band};
        }
    }

    // Draws
    if (res == "1-1") {
        if (ri_band == "High" && si_band == "Low")
            return {"SavedDraw", "High RI, low SI", ri_band, si_band};
        if (si_band == "High" && ri_band == "Low")
            return {"MissedWin", "High SI, low RI", ri_band, si_band};
        return {"QuietDraw", "Both RI and SI not extreme", ri_band, si_band};
    }

    // Non-draws (0-2 or 2-0)
    if (res == "0-2" || res == "2-0") {
        if (res == "0-2") { // loss
            if (si_band == "High" && ri_band != "High")
                return {"HadChances", "Significant winning spans before result slipped", ri_band, si_band};
            if (ri_band == "High")
                return {"BehindMostOfGame", "Consistently behind; high RI expected in losses", ri_band, si_band};
            return {"ControlledGame", "No big alternations or decisive blunder detected", ri_band, si_band};
        } else { // win
            if (ri_band == "High")
                return {"ComebackWin", "High RI suggests comeback", ri_band, si_band};
            if (si_band == "High")
                return {"BlunderWin", "Significant winning spans, no big setbacks", ri_band, si_band};
            return {"ControlledWin", "Stable win without large swings", ri_band, si_band};
        }
    }

    return {"ControlledGame", "Default classification", ri_band, si_band};
}

// ------------------------------- Printing ------------------------------------

static void pretty_print(const std::unordered_map<string,string>& headers,
                         const std::unordered_map<string,string>& params,
                         const Metrics& me,
                         const string& label,
                         const string& reason,
                         const string& ri_band,
                         const string& si_band,
                         int tedge) {
    cout << "=== PDN Luck Analysis ===\n";
    if (!headers.empty()) {
        auto get = [&](const char* k){ auto it = headers.find(k); return (it==headers.end()? string("Unknown"): it->second); };
        cout << "White : " << get("White") << "\n";
        cout << "Black : " << get("Black") << "\n";
        cout << "Result: " << get("Result") << "\n\n";
    }

    cout << "Parameters: tdraw=" << params.at("tdraw")
         << " | tedge=" << params.at("tedge")
         << " | severe=" << params.at("severe")
         << " | swing=" << params.at("swing_thresholds") << "\n\n";

    if (!me.error.empty()) { cout << "ERROR: " << me.error << "\n"; return; }

    cout << "Scored plies: " << me.n_scored << "   |   'Only move' plies: " << me.n_forced << "\n\n";
    cout << "Max disadvantage (most negative eval): " << me.min_eval << "\n";
    cout << "Max advantage (most positive eval):    " << me.max_eval << "\n";
    cout << "Final eval: " << me.final_eval << "   |   Comeback magnitude: " << me.comeback_magnitude << "\n";
    cout.setf(std::ios::fixed); cout.precision(2);
    cout << "Mean eval: " << me.mean_eval << "   |   Stdev: " << me.stdev_eval << "\n\n";
    cout.unsetf(std::ios::floatfield);
    cout.precision(6);

    auto pct = [&](double x){ std::ostringstream os; os.setf(std::ios::fixed); os.precision(1); os<< (x*100.0); return os.str(); };

    cout << "Time losing (<= -tedge):   " << me.num_losing << "/" << me.n_scored
         << " (" << pct(me.frac_losing) << "%)   |   Longest losing span: " << me.longest_losing_span << "\n";
    cout << "Time winning (>= +tedge):  " << me.num_winning << "/" << me.n_scored
         << " (" << pct(me.frac_winning) << "%)   |   Longest winning span: " << me.longest_winning_span << "\n";
    cout << "Severe losing (<= -severe): " << me.num_severe << "/" << me.n_scored
         << " (" << pct(me.frac_severe) << "%)   |   Longest severe span: " << me.longest_severe_span << "\n\n";

    cout << "Max single-ply swing |Δ|: " << me.max_swing << "\n";
    for (const auto& kv : me.swing_counts) {
        cout << "  Swings > " << kv.first << ": " << kv.second << "\n";
    }
    cout << "Lead changes (sign flips, zeros ignored): " << me.lead_changes << "\n\n";

    double ri = compute_rescue_index(me, tedge);
    double si = compute_squander_index(me);
    cout.setf(std::ios::fixed); cout.precision(3);
    cout << "Rescue Index (RI):   " << ri << " [" << ri_band << "]\n";
    cout << "Squander Index (SI): " << si << " [" << si_band << "]\n";
    cout.unsetf(std::ios::floatfield); cout.precision(6);
    cout << "  RI = 0.40*max(0, -min_eval)/60 + 0.25*frac_losing + 0.20*(longest_losing_span/8) + 0.15*(max_swing/50)\n";
    cout << "  SI = 0.40*max(0, +max_eval)/60 + 0.25*frac_winning + 0.20*(longest_winning_span/8) + 0.15*(max_swing/50)\n\n";

    cout << "Label: " << label << "\n";
    if (!reason.empty()) cout << "Reason: " << reason << "\n";
    cout << "\nForced-move context:\n";
    cout << "  Total 'only move' plies: " << me.forced_total << "\n";
    cout << "  Adjacent to losing (prev OR next scored <= -tedge): " << me.forced_adjacent_losing << "\n";
    cout << "  In losing tunnel (prev AND next scored <= -tedge):  " << me.forced_in_losing_tunnel << "\n";
}

// ------------------------------ CSV writer -----------------------------------

static string csv_escape(const string& s) {
    bool need_quotes = s.find_first_of(",\"\n\r") != string::npos;
    if (!need_quotes) return s;
    string out = "\"";
    for (char c : s) {
        if (c == '"') out += "\"\"";
        else out += c;
    }
    out += "\"";
    return out;
}

static void write_summary_csv(const string& path,
                              const vector<std::unordered_map<string,string>>& rows,
                              const vector<string>& fieldnames) {
    std::ofstream f(path, std::ios::out | std::ios::trunc);
    if (!f) { cerr << "Failed to write CSV: " << path << "\n"; return; }
    // header
    for (size_t i = 0; i < fieldnames.size(); ++i) {
        if (i) f << ",";
        f << csv_escape(fieldnames[i]);
    }
    f << "\n";
    // rows
    for (const auto& row : rows) {
        for (size_t i = 0; i < fieldnames.size(); ++i) {
            if (i) f << ",";
            auto it = row.find(fieldnames[i]);
            string v = (it == row.end() ? "" : it->second);
            f << csv_escape(v);
        }
        f << "\n";
    }
    f.close();
    cout << "\nWrote summary CSV: " << path << "\n";
}

// ------------------------------ Arg parsing ----------------------------------

struct Args {
    string pdn_file;
    int tdraw = 10;
    int tedge = 20;
    int severe = 40;
    vector<int> swing{10,20,30,40,50};
    optional<string> summary_csv;
    double high_threshold = 1.0;
    double low_threshold = 0.4;
    int blunder_threshold = 100;
    int decisive_span = 5;
};

static bool parse_int(const string& s, int& out) {
    try { out = std::stoi(s); return true; } catch (...) { return false; }
}

static bool parse_double(const string& s, double& out) {
    try { out = std::stod(s); return true; } catch (...) { return false; }
}

static void print_usage(const char* prog) {
    cerr <<
R"(Usage:
  )" << prog << R"( <file.pdn> [--tdraw 10] [--tedge 20] [--severe 40]
     [--swing 10,20,30,40,50] [--summary-csv out.csv]
     [--high-threshold 1.0] [--low-threshold 0.4]
     [--blunder-threshold 100] [--decisive-span 5]
)";
}

static bool parse_args(int argc, char** argv, Args& a) {
    if (argc < 2) { print_usage(argv[0]); return false; }

    auto parse_kv = [](const std::string& s) -> std::pair<std::string,std::optional<std::string>> {
        // returns {"--key", "value"} for "--key=value"
        auto pos = s.find('=');
        if (pos == std::string::npos) return {s, std::nullopt};
        return {s.substr(0, pos), s.substr(pos+1)};
    };

    auto need_next = [&](int& i, const char* name) -> std::string {
        if (i + 1 >= argc) { std::cerr << "Missing value for " << name << "\n"; std::exit(1); }
        return std::string(argv[++i]);
    };

    bool have_file = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        // Option with possible "=value"
        if (arg.rfind("--", 0) == 0) {
            auto [key, valopt] = parse_kv(arg);

            auto get_val = [&](const char* kname) -> std::string {
                if (valopt) return *valopt;
                return need_next(i, kname);
            };

            if (key == "--tdraw") {
                int v; if (!parse_int(get_val("--tdraw"), v)) { std::cerr<<"Bad --tdraw\n"; return false; }
                a.tdraw = v;
            } else if (key == "--tedge") {
                int v; if (!parse_int(get_val("--tedge"), v)) { std::cerr<<"Bad --tedge\n"; return false; }
                a.tedge = v;
            } else if (key == "--severe") {
                int v; if (!parse_int(get_val("--severe"), v)) { std::cerr<<"Bad --severe\n"; return false; }
                a.severe = v;
            } else if (key == "--swing") {
                std::string s = get_val("--swing");
                a.swing.clear();
                for (auto& p : split(s, ',')) {
                    std::string t = trim(p);
                    if (!t.empty()) {
                        int v; if (!parse_int(t, v)) { std::cerr << "Invalid --swing token: " << t << "\n"; return false; }
                        a.swing.push_back(v);
                    }
                }
            } else if (key == "--summary-csv") {
                a.summary_csv = get_val("--summary-csv");
            } else if (key == "--high-threshold") {
                double v; if (!parse_double(get_val("--high-threshold"), v)) { std::cerr<<"Bad --high-threshold\n"; return false; }
                a.high_threshold = v;
            } else if (key == "--low-threshold") {
                double v; if (!parse_double(get_val("--low-threshold"), v)) { std::cerr<<"Bad --low-threshold\n"; return false; }
                a.low_threshold = v;
            } else if (key == "--blunder-threshold") {
                int v; if (!parse_int(get_val("--blunder-threshold"), v)) { std::cerr<<"Bad --blunder-threshold\n"; return false; }
                a.blunder_threshold = v;
            } else if (key == "--decisive-span") {
                int v; if (!parse_int(get_val("--decisive-span"), v)) { std::cerr<<"Bad --decisive-span\n"; return false; }
                a.decisive_span = v;
            } else if (key == "--help" || key == "-h") {
                print_usage(argv[0]);
                std::exit(0);
            } else {
                std::cerr << "Unknown option: " << (valopt ? key + "=" + *valopt : key) << "\n";
                print_usage(argv[0]);
                return false;
            }
        } else {
            // Positional: PDN file (accept exactly one)
            if (!have_file) {
                a.pdn_file = arg;
                have_file = true;
            } else {
                std::cerr << "Unexpected extra positional argument: " << arg << "\n";
                print_usage(argv[0]);
                return false;
            }
        }
    }

    if (!have_file) {
        std::cerr << "Missing <file.pdn>\n";
        print_usage(argv[0]);
        return false;
    }
    return true;
}

// ------------------------------- main ----------------------------------------

int main(int argc, char** argv) {
    Args args;
    if (!parse_args(argc, argv, args)) return 1;

    // Read PDN file
    std::ifstream in(args.pdn_file, std::ios::in);
    if (!in) {
        cerr << "Cannot open PDN file: " << args.pdn_file << "\n";
        return 1;
    }
    std::stringstream buffer;
    buffer << in.rdbuf();
    string pdn_text = buffer.str();

    auto games = split_games(pdn_text);
    cout << "Found " << games.size() << " game(s) in " << args.pdn_file << "\n\n";

    vector<std::unordered_map<string,string>> summaries;

    for (size_t i = 0; i < games.size(); ++i) {
        const string& game_text = games[i];
        auto headers = parse_headers(game_text);
        auto entries = parse_entries(game_text);
        auto metrics = analyze(entries, args.tdraw, args.tedge, args.severe, args.swing);

        cout << string(80, '=') << "\n";
        cout << "Game " << (i + 1) << "\n";
        cout << string(80, '=') << "\n";

        string result = "";
        auto itR = headers.find("Result");
        if (itR != headers.end()) result = itR->second;

        auto [label, reason, ri_band, si_band] =
            label_game(result, metrics, args.high_threshold, args.low_threshold,
                       args.blunder_threshold, args.decisive_span, args.tedge);

        std::unordered_map<string,string> params = {
            {"tdraw", std::to_string(args.tdraw)},
            {"tedge", std::to_string(args.tedge)},
            {"severe", std::to_string(args.severe)}
        };
        // Join swing thresholds for pretty print
        {
            std::ostringstream os;
            for (size_t k = 0; k < args.swing.size(); ++k) {
                if (k) os << ",";
                os << args.swing[k];
            }
            params["swing_thresholds"] = os.str();
        }

        pretty_print(headers, params, metrics, label, reason, ri_band, si_band, args.tedge);

        // Build summary row
        double ri = compute_rescue_index(metrics, args.tedge);
        double si = compute_squander_index(metrics);

        std::unordered_map<string,string> row;
        row["Game"]  = std::to_string(i + 1);
        row["White"] = headers.count("White") ? headers["White"] : "";
        row["Black"] = headers.count("Black") ? headers["Black"] : "";
        row["Result"]= headers.count("Result")? headers["Result"]: "";

        auto add = [&](const string& k, const string& v){ row[k]=v; };
        auto addi = [&](const string& k, int v){ row[k]=std::to_string(v); };
        auto addd3 = [&](const string& k, double v){ std::ostringstream os; os.setf(std::ios::fixed); os.precision(3); os<<v; row[k]=os.str(); };

        addi("n_scored", metrics.n_scored);
        addi("n_forced", metrics.n_forced);
        addi("min_eval", metrics.min_eval);
        addi("max_eval", metrics.max_eval);
        addi("final_eval", metrics.final_eval);
        addd3("mean_eval", metrics.mean_eval);
        addd3("stdev_eval", metrics.stdev_eval);
        addi("num_losing", metrics.num_losing);
        addd3("frac_losing", metrics.frac_losing);
        addi("longest_losing_span", metrics.longest_losing_span);
        addi("num_winning", metrics.num_winning);
        addd3("frac_winning", metrics.frac_winning);
        addi("longest_winning_span", metrics.longest_winning_span);
        addi("num_severe", metrics.num_severe);
        addd3("frac_severe", metrics.frac_severe);
        addi("longest_severe_span", metrics.longest_severe_span);
        addi("comeback_magnitude", metrics.comeback_magnitude);
        addi("max_swing", metrics.max_swing);
        addi("lead_changes", metrics.lead_changes);
        addi("forced_total", metrics.forced_total);
        addi("forced_adjacent_losing", metrics.forced_adjacent_losing);
        addi("forced_in_losing_tunnel", metrics.forced_in_losing_tunnel);

        // swing_counts as ">thr:count; ..."
        {
            std::ostringstream os;
            bool first = true;
            for (const auto& kv : metrics.swing_counts) {
                if (!first) os << "; ";
                first = false;
                os << ">" << kv.first << ":" << kv.second;
            }
            add("swing_counts", os.str());
        }

        addd3("rescue_index", ri);
        addd3("squander_index", si);
        add("RI_band", ri_band);
        add("SI_band", si_band);
        add("label", label);

        summaries.push_back(std::move(row));
    }

    if (args.summary_csv.has_value()) {
        vector<string> fieldnames = {
            "Game","White","Black","Result",
            "n_scored","n_forced",
            "min_eval","max_eval","final_eval","mean_eval","stdev_eval",
            "num_losing","frac_losing","longest_losing_span",
            "num_winning","frac_winning","longest_winning_span",
            "num_severe","frac_severe","longest_severe_span",
            "comeback_magnitude","max_swing","lead_changes",
            "forced_total","forced_adjacent_losing","forced_in_losing_tunnel",
            "swing_counts","rescue_index","squander_index","RI_band","SI_band","label"
        };
        write_summary_csv(*args.summary_csv, summaries, fieldnames);
    }

    return 0;
}


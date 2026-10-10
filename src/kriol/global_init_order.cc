#include "../../include/kriol/global_init_order.hh"

#include <algorithm>
#include <queue>
#include <unordered_set>

namespace kriol {
namespace sema {

namespace {

const std::vector<std::string>& entriesOf(
        const std::unordered_map<std::string, std::vector<std::string>>& table,
        const std::string& key) {
    static const std::vector<std::string> none;
    auto found = table.find(key);
    return found == table.end() ? none : found->second;
}

std::vector<std::string> callPath(const std::unordered_map<std::string, std::string>& caller,
                                  std::string function) {
    std::vector<std::string> path;
    while (!function.empty()) {
        path.push_back(function);
        function = caller.at(function);
    }
    std::reverse(path.begin(), path.end());
    return path;
}

std::string describe(const std::string& global, const std::vector<std::string>& path,
                     const std::string& used) {
    std::string message = "o valor inicial de '" + global + "' chama '" + path.front() + "'";
    for (std::size_t i = 1; i < path.size(); ++i)
        message += ", que chama '" + path[i] + "'";
    if (used == global)
        return message + ", que usa a própria variável '" + used
               + "' antes de ela receber o seu valor inicial";
    return message + ", que usa a variável '" + used + "' antes de ela receber o seu valor inicial; "
           "as variáveis fora das funções recebem o valor pela ordem em que são declaradas, "
           "por isso declara '" + used + "' antes de '" + global + "'";
}

} // namespace

void GlobalInitOrder::declareGlobal(const std::string& name) {
    DeclarationOrder.emplace(name, DeclarationOrder.size());
}

void GlobalInitOrder::noteGlobalUse(const std::string& function, const std::string& global) {
    GlobalUses[function].push_back(global);
}

void GlobalInitOrder::noteCall(const std::string& caller, const std::string& callee) {
    Callees[caller].push_back(callee);
}

void GlobalInitOrder::noteInitializerCall(const std::string& global, const std::string& callee,
                                          int lineNum) {
    InitializerCalls.push_back({global, callee, lineNum});
}

std::vector<GlobalInitOrder::Violation> GlobalInitOrder::violations() const {
    std::vector<Violation> found;
    std::unordered_set<std::string> reported;

    for (const auto& call : InitializerCalls) {
        auto initializing = DeclarationOrder.find(call.global);
        if (initializing == DeclarationOrder.end()) continue;

        std::unordered_map<std::string, std::string> caller{{call.callee, ""}};
        std::queue<std::string> pending;
        pending.push(call.callee);

        while (!pending.empty()) {
            const std::string function = pending.front();
            pending.pop();

            for (const auto& used : entriesOf(GlobalUses, function)) {
                auto order = DeclarationOrder.find(used);
                if (order == DeclarationOrder.end() || order->second < initializing->second) continue;
                if (!reported.insert(call.global + '\n' + used).second) continue;
                found.push_back({call.lineNum, describe(call.global, callPath(caller, function), used)});
            }

            for (const auto& next : entriesOf(Callees, function))
                if (caller.emplace(next, function).second) pending.push(next);
        }
    }
    return found;
}

} // namespace sema
} // namespace kriol

#ifndef KRIOL_GLOBAL_INIT_ORDER_HEADER
#define KRIOL_GLOBAL_INIT_ORDER_HEADER

#include <cstddef>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace kriol {
namespace sema {

class GlobalInitOrder {
public:
    struct Violation {
        int lineNum;
        std::string message;
    };

    void declareGlobal(const std::string& name);
    void noteGlobalUse(const std::string& function, const std::string& global);
    void noteCall(const std::string& caller, const std::string& callee);
    void noteInitializerCall(const std::string& global, const std::string& callee, int lineNum);

    std::vector<Violation> violations() const;

private:
    struct InitializerCall {
        std::string global;
        std::string callee;
        int lineNum;
    };

    struct LatestUse {
        std::size_t order;
        std::string global;
        std::vector<std::string> path;
    };

    std::optional<LatestUse> latestUseReachedFrom(const std::string& root) const;

    std::unordered_map<std::string, std::size_t> DeclarationOrder;
    std::unordered_map<std::string, std::vector<std::string>> GlobalUses;
    std::unordered_map<std::string, std::vector<std::string>> Callees;
    std::vector<InitializerCall> InitializerCalls;
};

} // namespace sema
} // namespace kriol

#endif // KRIOL_GLOBAL_INIT_ORDER_HEADER

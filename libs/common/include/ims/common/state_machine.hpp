/**
 * @file state_machine.hpp
 * @brief Generic finite state machine implementation
 * @version 3.6.2
 *
 * Provides state machine functionality including:
 * - Generic finite state machine
 * - Event-driven state transitions
 * - Guard conditions and actions
 * - State entry/exit callbacks
 *
 * Copyright (c) 2025 Bennie Shearer
 * MIT License - See LICENSE file for details
 */

#ifndef IMS_COMMON_STATE_MACHINE_HPP
#define IMS_COMMON_STATE_MACHINE_HPP

#include "types.hpp"
#include <functional>
#include <variant>
#include <any>

namespace ims::common {

// =============================================================================
// State Machine Types
// =============================================================================

/**
 * @brief Result of a state transition
 */
enum class TransitionResult {
    Success,        ///< Transition completed
    GuardFailed,    ///< Guard condition returned false
    InvalidEvent,   ///< Event not valid for current state
    InvalidState,   ///< Target state does not exist
    ActionFailed    ///< Action threw an exception
};

/**
 * @brief Converts transition result to string
 */
inline String transition_result_to_string(TransitionResult result) {
    switch (result) {
        case TransitionResult::Success: return "Success";
        case TransitionResult::GuardFailed: return "GuardFailed";
        case TransitionResult::InvalidEvent: return "InvalidEvent";
        case TransitionResult::InvalidState: return "InvalidState";
        case TransitionResult::ActionFailed: return "ActionFailed";
        default: return "Unknown";
    }
}

// =============================================================================
// State Definition
// =============================================================================

/**
 * @brief Definition of a state in the state machine
 */
template<typename StateId, typename Event, typename Context>
struct StateDefinition {
    using OnEnterCallback = Function<void(Context&, const Event*)>;
    using OnExitCallback = Function<void(Context&, const Event*)>;
    using OnUpdateCallback = Function<void(Context&)>;
    
    StateId id;
    String name;
    OnEnterCallback on_enter;
    OnExitCallback on_exit;
    OnUpdateCallback on_update;
    bool is_terminal{false};
    
    StateDefinition() = default;
    explicit StateDefinition(StateId state_id, String state_name = "")
        : id(state_id), name(std::move(state_name)) {}
};

// =============================================================================
// Transition Definition
// =============================================================================

/**
 * @brief Definition of a transition between states
 */
template<typename StateId, typename Event, typename Context>
struct TransitionDefinition {
    using GuardCallback = Function<bool(const Context&, const Event&)>;
    using ActionCallback = Function<void(Context&, const Event&)>;
    
    StateId from;
    StateId to;
    Function<bool(const Event&)> event_matcher;
    GuardCallback guard;
    ActionCallback action;
    String description;
    
    TransitionDefinition() = default;
    
    TransitionDefinition(StateId from_state, StateId to_state)
        : from(from_state), to(to_state) {}
};

// =============================================================================
// State Machine History
// =============================================================================

/**
 * @brief Record of a state transition
 */
template<typename StateId, typename Event>
struct TransitionRecord {
    StateId from_state;
    StateId to_state;
    Event event;
    SystemTimePoint timestamp;
    TransitionResult result;
};

// =============================================================================
// State Machine
// =============================================================================

/**
 * @brief Generic finite state machine
 * @tparam StateId Type used to identify states (e.g., enum, int, string)
 * @tparam Event Type used for events (can be variant for multiple event types)
 * @tparam Context Type for shared context data
 */
template<typename StateId, typename Event, typename Context = std::any>
class StateMachine {
public:
    using State = StateDefinition<StateId, Event, Context>;
    using Transition = TransitionDefinition<StateId, Event, Context>;
    using Record = TransitionRecord<StateId, Event>;
    using StateChangeCallback = Function<void(StateId, StateId, const Event&)>;
    
private:
    Map<StateId, State> states_;
    Vector<Transition> transitions_;
    StateId current_state_;
    StateId initial_state_;
    Context context_;
    mutable Mutex mutex_;
    
    bool started_{false};
    Vector<Record> history_;
    Size max_history_{100};
    StateChangeCallback on_state_change_;
    
    void record_transition(StateId from, StateId to, const Event& event, 
                          TransitionResult result) {
        Record record{from, to, event, SystemClock::now(), result};
        
        if (history_.size() >= max_history_) {
            history_.erase(history_.begin());
        }
        history_.push_back(std::move(record));
    }
    
    const Transition* find_transition(const Event& event) const {
        for (const auto& trans : transitions_) {
            if (trans.from == current_state_) {
                if (!trans.event_matcher || trans.event_matcher(event)) {
                    return &trans;
                }
            }
        }
        return nullptr;
    }
    
public:
    StateMachine() = default;
    
    explicit StateMachine(const Context& initial_context) 
        : context_(initial_context) {}
    
    /**
     * @brief Adds a state to the machine
     */
    void add_state(State state) {
        LockGuard<Mutex> lock(mutex_);
        states_[state.id] = std::move(state);
    }
    
    /**
     * @brief Adds a state with callbacks
     */
    void add_state(StateId id, const String& name = "",
                   typename State::OnEnterCallback on_enter = nullptr,
                   typename State::OnExitCallback on_exit = nullptr) {
        State state(id, name);
        state.on_enter = std::move(on_enter);
        state.on_exit = std::move(on_exit);
        add_state(std::move(state));
    }
    
    /**
     * @brief Adds a terminal state
     */
    void add_terminal_state(StateId id, const String& name = "",
                            typename State::OnEnterCallback on_enter = nullptr) {
        State state(id, name);
        state.on_enter = std::move(on_enter);
        state.is_terminal = true;
        add_state(std::move(state));
    }
    
    /**
     * @brief Adds a transition between states
     */
    void add_transition(Transition trans) {
        LockGuard<Mutex> lock(mutex_);
        transitions_.push_back(std::move(trans));
    }
    
    /**
     * @brief Adds a simple transition
     */
    void add_transition(StateId from, StateId to,
                        Function<bool(const Event&)> event_matcher = nullptr,
                        typename Transition::GuardCallback guard = nullptr,
                        typename Transition::ActionCallback action = nullptr) {
        Transition trans(from, to);
        trans.event_matcher = std::move(event_matcher);
        trans.guard = std::move(guard);
        trans.action = std::move(action);
        add_transition(std::move(trans));
    }
    
    /**
     * @brief Sets the initial state
     */
    void set_initial_state(StateId state) {
        LockGuard<Mutex> lock(mutex_);
        initial_state_ = state;
    }
    
    /**
     * @brief Starts the state machine
     */
    bool start() {
        LockGuard<Mutex> lock(mutex_);
        
        if (started_) return false;
        
        auto it = states_.find(initial_state_);
        if (it == states_.end()) return false;
        
        current_state_ = initial_state_;
        started_ = true;
        
        // Call on_enter for initial state
        if (it->second.on_enter) {
            it->second.on_enter(context_, nullptr);
        }
        
        return true;
    }
    
    /**
     * @brief Processes an event
     * @return Result of the transition attempt
     */
    TransitionResult process_event(const Event& event) {
        LockGuard<Mutex> lock(mutex_);
        
        if (!started_) {
            return TransitionResult::InvalidState;
        }
        
        // Check if current state is terminal
        auto current_it = states_.find(current_state_);
        if (current_it != states_.end() && current_it->second.is_terminal) {
            record_transition(current_state_, current_state_, event, 
                            TransitionResult::InvalidEvent);
            return TransitionResult::InvalidEvent;
        }
        
        // Find matching transition
        const Transition* trans = find_transition(event);
        if (!trans) {
            record_transition(current_state_, current_state_, event, 
                            TransitionResult::InvalidEvent);
            return TransitionResult::InvalidEvent;
        }
        
        // Check guard condition
        if (trans->guard && !trans->guard(context_, event)) {
            record_transition(current_state_, trans->to, event, 
                            TransitionResult::GuardFailed);
            return TransitionResult::GuardFailed;
        }
        
        // Verify target state exists
        auto target_it = states_.find(trans->to);
        if (target_it == states_.end()) {
            record_transition(current_state_, trans->to, event, 
                            TransitionResult::InvalidState);
            return TransitionResult::InvalidState;
        }
        
        StateId from_state = current_state_;
        
        try {
            // Exit current state
            if (current_it != states_.end() && current_it->second.on_exit) {
                current_it->second.on_exit(context_, &event);
            }
            
            // Execute transition action
            if (trans->action) {
                trans->action(context_, event);
            }
            
            // Enter new state
            current_state_ = trans->to;
            
            if (target_it->second.on_enter) {
                target_it->second.on_enter(context_, &event);
            }
            
            // Record and notify
            record_transition(from_state, current_state_, event, 
                            TransitionResult::Success);
            
            if (on_state_change_) {
                on_state_change_(from_state, current_state_, event);
            }
            
            return TransitionResult::Success;
            
        } catch (...) {
            record_transition(from_state, trans->to, event, 
                            TransitionResult::ActionFailed);
            return TransitionResult::ActionFailed;
        }
    }
    
    /**
     * @brief Updates the current state (for continuous processing)
     */
    void update() {
        LockGuard<Mutex> lock(mutex_);
        
        if (!started_) return;
        
        auto it = states_.find(current_state_);
        if (it != states_.end() && it->second.on_update) {
            it->second.on_update(context_);
        }
    }
    
    /**
     * @brief Forces a transition to a specific state
     */
    bool force_transition(StateId to_state) {
        LockGuard<Mutex> lock(mutex_);
        
        if (!started_) return false;
        
        auto target_it = states_.find(to_state);
        if (target_it == states_.end()) return false;
        
        auto current_it = states_.find(current_state_);
        
        // Exit current state
        if (current_it != states_.end() && current_it->second.on_exit) {
            current_it->second.on_exit(context_, nullptr);
        }
        
        StateId from_state = current_state_;
        current_state_ = to_state;
        
        // Enter new state
        if (target_it->second.on_enter) {
            target_it->second.on_enter(context_, nullptr);
        }
        
        return true;
    }
    
    /**
     * @brief Resets the state machine to initial state
     */
    void reset() {
        LockGuard<Mutex> lock(mutex_);
        
        if (started_) {
            auto current_it = states_.find(current_state_);
            if (current_it != states_.end() && current_it->second.on_exit) {
                current_it->second.on_exit(context_, nullptr);
            }
        }
        
        started_ = false;
        history_.clear();
    }
    
    /**
     * @brief Gets the current state
     */
    StateId current() const {
        LockGuard<Mutex> lock(mutex_);
        return current_state_;
    }
    
    /**
     * @brief Checks if the machine is in a specific state
     */
    bool is_in_state(StateId state) const {
        LockGuard<Mutex> lock(mutex_);
        return current_state_ == state;
    }
    
    /**
     * @brief Checks if the machine is in a terminal state
     */
    bool is_terminated() const {
        LockGuard<Mutex> lock(mutex_);
        auto it = states_.find(current_state_);
        return it != states_.end() && it->second.is_terminal;
    }
    
    /**
     * @brief Checks if the machine has been started
     */
    bool is_started() const {
        LockGuard<Mutex> lock(mutex_);
        return started_;
    }
    
    /**
     * @brief Gets the context
     */
    Context& context() { return context_; }
    const Context& context() const { return context_; }
    
    /**
     * @brief Sets the state change callback
     */
    void on_state_change(StateChangeCallback callback) {
        LockGuard<Mutex> lock(mutex_);
        on_state_change_ = std::move(callback);
    }
    
    /**
     * @brief Gets the transition history
     */
    Vector<Record> get_history() const {
        LockGuard<Mutex> lock(mutex_);
        return history_;
    }
    
    /**
     * @brief Gets all valid events for the current state
     */
    Vector<const Transition*> valid_transitions() const {
        LockGuard<Mutex> lock(mutex_);
        Vector<const Transition*> result;
        
        for (const auto& trans : transitions_) {
            if (trans.from == current_state_) {
                result.push_back(&trans);
            }
        }
        
        return result;
    }
    
    /**
     * @brief Gets the state name
     */
    String state_name(StateId id) const {
        LockGuard<Mutex> lock(mutex_);
        auto it = states_.find(id);
        return it != states_.end() ? it->second.name : "";
    }
    
    /**
     * @brief Gets current state name
     */
    String current_state_name() const {
        return state_name(current());
    }
    
    /**
     * @brief Sets maximum history size
     */
    void set_max_history(Size size) {
        LockGuard<Mutex> lock(mutex_);
        max_history_ = size;
        while (history_.size() > max_history_) {
            history_.erase(history_.begin());
        }
    }
    
    /**
     * @brief Gets all state IDs
     */
    Vector<StateId> state_ids() const {
        LockGuard<Mutex> lock(mutex_);
        Vector<StateId> ids;
        for (const auto& [id, _] : states_) {
            ids.push_back(id);
        }
        return ids;
    }
};

// =============================================================================
// State Machine Builder
// =============================================================================

/**
 * @brief Builder pattern for state machine construction
 */
template<typename StateId, typename Event, typename Context = std::any>
class StateMachineBuilder {
private:
    using Machine = StateMachine<StateId, Event, Context>;
    using State = typename Machine::State;
    using Transition = typename Machine::Transition;
    
    UniquePtr<Machine> machine_;
    
public:
    StateMachineBuilder() : machine_(std::make_unique<Machine>()) {}
    
    explicit StateMachineBuilder(const Context& initial_context) 
        : machine_(std::make_unique<Machine>(initial_context)) {}
    
    /**
     * @brief Adds a state
     */
    StateMachineBuilder& state(StateId id, const String& name = "") {
        machine_->add_state(id, name);
        return *this;
    }
    
    /**
     * @brief Adds a state with enter callback
     */
    StateMachineBuilder& state(StateId id, const String& name,
                               typename State::OnEnterCallback on_enter) {
        machine_->add_state(id, name, std::move(on_enter));
        return *this;
    }
    
    /**
     * @brief Adds a state with enter and exit callbacks
     */
    StateMachineBuilder& state(StateId id, const String& name,
                               typename State::OnEnterCallback on_enter,
                               typename State::OnExitCallback on_exit) {
        machine_->add_state(id, name, std::move(on_enter), std::move(on_exit));
        return *this;
    }
    
    /**
     * @brief Adds a terminal state
     */
    StateMachineBuilder& terminal_state(StateId id, const String& name = "",
                                        typename State::OnEnterCallback on_enter = nullptr) {
        machine_->add_terminal_state(id, name, std::move(on_enter));
        return *this;
    }
    
    /**
     * @brief Sets the initial state
     */
    StateMachineBuilder& initial(StateId id) {
        machine_->set_initial_state(id);
        return *this;
    }
    
    /**
     * @brief Adds a transition
     */
    StateMachineBuilder& transition(StateId from, StateId to,
                                    Function<bool(const Event&)> matcher = nullptr) {
        machine_->add_transition(from, to, std::move(matcher));
        return *this;
    }
    
    /**
     * @brief Adds a guarded transition
     */
    StateMachineBuilder& guarded_transition(
            StateId from, StateId to,
            Function<bool(const Event&)> matcher,
            typename Transition::GuardCallback guard) {
        machine_->add_transition(from, to, std::move(matcher), std::move(guard));
        return *this;
    }
    
    /**
     * @brief Adds a transition with action
     */
    StateMachineBuilder& transition_with_action(
            StateId from, StateId to,
            Function<bool(const Event&)> matcher,
            typename Transition::ActionCallback action) {
        machine_->add_transition(from, to, std::move(matcher), nullptr, 
                                std::move(action));
        return *this;
    }
    
    /**
     * @brief Builds the state machine
     */
    UniquePtr<Machine> build() {
        return std::move(machine_);
    }
};

// =============================================================================
// Hierarchical State (Optional Extension)
// =============================================================================

/**
 * @brief State that can contain sub-states (composite state)
 */
template<typename StateId, typename Event, typename Context>
class HierarchicalState {
private:
    StateId id_;
    String name_;
    UniquePtr<StateMachine<StateId, Event, Context>> sub_machine_;
    StateId parent_id_;
    bool has_parent_{false};
    
public:
    HierarchicalState(StateId id, String name = "") 
        : id_(id), name_(std::move(name)) {}
    
    void set_sub_machine(UniquePtr<StateMachine<StateId, Event, Context>> machine) {
        sub_machine_ = std::move(machine);
    }
    
    void set_parent(StateId parent) {
        parent_id_ = parent;
        has_parent_ = true;
    }
    
    bool has_sub_machine() const { return sub_machine_ != nullptr; }
    bool has_parent() const { return has_parent_; }
    
    StateId id() const { return id_; }
    const String& name() const { return name_; }
    StateId parent() const { return parent_id_; }
    
    StateMachine<StateId, Event, Context>* sub_machine() { 
        return sub_machine_.get(); 
    }
    
    const StateMachine<StateId, Event, Context>* sub_machine() const { 
        return sub_machine_.get(); 
    }
};

} // namespace ims::common

#endif // IMS_COMMON_STATE_MACHINE_HPP

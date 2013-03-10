#pragma once

#include <set>
#include <vector>

#include "mcsat/variable/variable.h"
#include "mcsat/clause/clause_ref.h"

namespace CVC4 {
namespace mcsat {

class SolverPlugin;
  
/** 
 * Kinds of notification a solver can get
 */
enum NotificationType {
  /** Notify of a restart */
  NOTIFY_RESTART,
  /** Notify of a conflict */
  NOTIFY_CONFLICT,
  /** Notify of a conflicr resolution step */
  NOTIFY_CONFLICT_RESOLUTION,
  /** Notify of vaible being unset */
  NOTIFY_VARIABLE_UNSET
};

/** Interface for plugin notificaiton */
class SolverPluginNotify {

public:
  
  typedef std::set<NotificationType> notification_set;

private:
  
  /** Which notifications the plugin asks for */
  notification_set d_notifications;
  
protected:
  
  /** To be called by plugins to subscribe */
  void addNotification(NotificationType type) {
    d_notifications.insert(type);
  }

public:
  
  /** Get all notification types */
  const notification_set& getNotificationTypes() {
    return d_notifications;
  }

  /** Notification of a restart */
  virtual void notifyRestart() {
    Unreachable("If you subscribe, then reimplement");
  }

  /** Notification of a new conflict */
  virtual void notifyConflict() {
    Unreachable("If you subscribe, then reimplement");
  }

  /** Nofification of a new conflict resolution step */
  virtual void notifyConflictResolution(CRef clause) {
    Unreachable("If you subscribe, then reimplement");
  }
  
  /** Notification of unset variables */
  virtual void notifyVariableUnset(const std::vector<Variable>& vars) {
    Unreachable("If you subscribe, then reimplement");
  }
};


/**
 * Class to collect plugins and notify them.
 */
class NotificationDispatch : public SolverPluginNotify  {

  /** All plugins arranged by notifications */
  std::vector< std::vector<SolverPlugin*> > d_toNotify;

public:
  
  /** Add a plugin to notify */
  void addPlugin(SolverPlugin* plugin);
  
  /** Notification of a new conflict */
  void notifyRestart();

  /** Notification of a new conflict */
  void notifyConflict();
  
  /** Nofification of a new conflict resolution step */
  void notifyConflictResolution(CRef clause);
  
  /** Notification of unset variables */
  void notifyVariableUnset(const std::vector<Variable>& vars);
};

}
}
#ifndef USERROLE_H
#define USERROLE_H

/**
 * UserRole - Defines the access level for the current user.
 *
 * Commander: Full design capabilities on the System Overview canvas.
 *            Can drag-drop components, draw connections, save/load designs.
 *            Sees only the System Overview canvas (no per-component tabs).
 *
 * Operator:  Monitor-only access. Can load and view designs on the System
 *            Overview canvas and analytics in real-time, but cannot save,
 *            clear, add types, or use connection mode. Does not see the
 *            Components panel. Has per-component enlarged view tabs with
 *            data analytics.
 */
enum class UserRole {
    Commander,  // Design + monitor on System Overview canvas only
    Operator    // Monitor-only, no components panel, no design tools
};

#endif // USERROLE_H

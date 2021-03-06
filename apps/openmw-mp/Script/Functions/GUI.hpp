//
// Created by koncord on 30.08.16.
//

#ifndef OPENMW_GUIAPI_HPP
#define OPENMW_GUIAPI_HPP

#define GUIAPI \
    {"MessageBox",          GUIFunctions::_MessageBox},\
    {"CustomMessageBox",    GUIFunctions::CustomMessageBox},\
    {"InputDialog",         GUIFunctions::InputDialog},\
    {"PasswordDialog",      GUIFunctions::PasswordDialog},\
    {"ListBox",             GUIFunctions::ListBox},\
    {"SetMapVisibility",    GUIFunctions::SetMapVisibility},\
    {"SetMapVisibilityAll", GUIFunctions::SetMapVisibilityAll}

class GUIFunctions
{
public:
    /* Do not rename into MessageBox to not conflict with WINAPI's MessageBox */
    static void _MessageBox(unsigned short pid, int id, const char *label) noexcept;

    static void CustomMessageBox(unsigned short pid, int id, const char *label, const char *buttons) noexcept;
    static void InputDialog(unsigned short pid, int id, const char *label) noexcept;
    static void PasswordDialog(unsigned short pid, int id, const char *label, const char *note) noexcept;

    static void ListBox(unsigned short pid, int id, const char *label, const char *items);

    //state 0 - disallow, 1 - allow
    static void SetMapVisibility(unsigned short targetPID, unsigned short affectedPID, unsigned short state) noexcept;
    static void SetMapVisibilityAll(unsigned short targetPID, unsigned short state) noexcept;
};

#endif //OPENMW_GUIAPI_HPP

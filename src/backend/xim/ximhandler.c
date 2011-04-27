/***************************************************************************
 *   Copyright (C) 2010~2010 by CSSlayer                                   *
 *   wengxt@gmail.com                                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <X11/Xutil.h>

#include "core/ime-internal.h"
#include "xim.h"
#include "Xi18n.h"
#include "IC.h"
#include "fcitx-config/hotkey.h"
#include "fcitx-config/cutils.h"
#include <core/ui.h>

extern FcitxXimBackend xim;

static void SetTrackPos(IMChangeICStruct * call_data);

Bool XIMOpenHandler(IMOpenStruct * call_data)
{
    return True;
}


Bool XIMGetICValuesHandler(IMChangeICStruct * call_data)
{
    XimGetIC(call_data);

    return True;
}

Bool XIMSetICValuesHandler(IMChangeICStruct * call_data)
{
    XimSetIC(call_data);
    SetTrackPos(call_data);

    return True;
}

Bool XIMSetFocusHandler(IMChangeFocusStruct * call_data)
{
    FcitxInputContext* ic =  FindIC(backend.backendid, &call_data->icid);
    SetCurrentIC(ic);
    
    if (ic && ic->state != IS_CLOSED)
    {
        OnInputFocus();
    }
    else
    {
        CloseInputWindow();
        MoveInputWindow();
    }

    return True;
}

Bool XIMUnsetFocusHandler(IMChangeICStruct * call_data)
{
    FcitxInputContext* ic = GetCurrentIC();
    if (ic && GetXimIC(ic)->id == call_data->icid)
    {
        CloseInputWindow();
    }

    return True;
}

Bool XIMCloseHandler(IMOpenStruct * call_data)
{
    CloseInputWindow();
    SaveAllIM();
    return True;
}

Bool XIMCreateICHandler(IMChangeICStruct * call_data)
{
    CreateIC(backend.backendid, call_data);
    FcitxInputContext* ic = GetCurrentIC();

    if (!ic) {
        ic = FindIC(backend.backendid, &call_data->icid);
        SetCurrentIC(ic);
    }

    return True;
}

Bool XIMDestroyICHandler(IMChangeICStruct * call_data)
{
    if (GetCurrentIC() == FindIC(backend.backendid, &call_data->icid)) {
    }

    DestroyIC(backend.backendid, &call_data->icid);

    return True;
}

Bool XIMTriggerNotifyHandler(IMTriggerNotifyStruct * call_data)
{
    FcitxInputContext* ic = FindIC(backend.backendid, &call_data->icid);
    if (ic == NULL)
        return True;

    SetCurrentIC(ic);
    return True;
}


void SetTrackPos(IMChangeICStruct * call_data)
{
    Bool flag = False;
    FcitxInputContext* ic = GetCurrentIC();
    if (ic == NULL)
        return;
    if (ic != FindIC(backend.backendid, &call_data->icid))
        return;

    int i;
    XICAttribute *pre_attr = ((IMChangeICStruct *) call_data)->preedit_attr;

    for (i = 0; i < (int) ((IMChangeICStruct *) call_data)->preedit_attr_num; i++, pre_attr++) {
        if (!strcmp(XNSpotLocation, pre_attr->name)) {
            flag = True;
            
            ic->offset_x = (*(XPoint *) pre_attr->value).x;
            ic->offset_y = (*(XPoint *) pre_attr->value).y;
        }
    }
    FcitxXimIC* ximic = GetXimIC(ic);
    
    Window window = None;
    if (ximic->focus_win)
        window = ximic->focus_win;
    else if(ximic->client_win)
        window = ximic->client_win;

    if(window != None && (ic->offset_x < 0 || ic->offset_y < 0))
    {
            
        XWindowAttributes attr;
        XGetWindowAttributes(xim.display, window, &attr);

        ic->offset_x = 0;
        ic->offset_y = attr.height;
    }

    MoveInputWindow();
}

void XIMProcessKey(IMForwardEventStruct * call_data)
{
    KeySym sym;
    XKeyEvent *kev;
    int keyCount;
    unsigned int state;
    char strbuf[STRBUFLEN];
    FcitxInputContext* ic = GetCurrentIC();
 
    if (!ic) {
        ic = FindIC(backend.backendid, &call_data->icid);
        SetCurrentIC(ic);
        if (!ic)
            return;
    }

    if (GetXimIC(ic)->id != call_data->icid) {
        ic = FindIC(backend.backendid, &call_data->icid);
        if (!ic)
            return;
    }

    kev = (XKeyEvent *) & call_data->event;
    memset(strbuf, 0, STRBUFLEN);
    keyCount = XLookupString(kev, strbuf, STRBUFLEN, &sym, NULL);

    state = kev->state - (kev->state & KEY_NUMLOCK) - (kev->state & KEY_CAPSLOCK) - (kev->state & KEY_SCROLLLOCK);
    GetKey(sym, state, keyCount, &sym, &state);
    FcitxLog(DEBUG,
        "KeyRelease=%d  state=%d  KEYCODE=%d  KEYSYM=%d  keyCount=%d",
         (call_data->event.type == KeyRelease), state, kev->keycode, (int) sym, keyCount);

    ProcessKey((call_data->event.type == KeyRelease)?(FCITX_RELEASE_KEY):(FCITX_PRESS_KEY),
                                        kev->time,
                                        sym, state);
}

void XIMClose(FcitxInputContext* ic, FcitxKeySym sym, unsigned int state, int count)
{
    if (ic == NULL)
        return;
    IMChangeFocusStruct call_data;

    call_data.connect_id = GetXimIC(ic)->connect_id;
    call_data.icid = GetXimIC(ic)->id;
 
    IMPreeditEnd(xim.ims, (XPointer) &call_data);
}
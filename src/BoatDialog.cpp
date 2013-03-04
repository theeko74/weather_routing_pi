/******************************************************************************
 *
 * Project:  OpenCPN
 * Purpose:  Weather Routing Plugin
 * Author:   Sean D'Epagnier
 *
 ***************************************************************************
 *   Copyright (C) 2013 by Sean D'Epagnier   *
 *   sean@depagnier.com   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,  USA.         *
 ***************************************************************************
 *
 */
#include "wx/wx.h"
#include "wx/tokenzr.h"
#include "wx/datetime.h"
#include "wx/sound.h"
#include <wx/wfstream.h>
#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/debug.h>
#include <wx/graphics.h>

#include <stdlib.h>
#include <math.h>
#include <time.h>

#include "Utilities.h"
#include "BoatSpeed.h"
#include "BoatDialog.h"

//---------------------------------------------------------------------------------------
//          Weather Routing Dialog Implementation
//---------------------------------------------------------------------------------------


BoatDialog::BoatDialog( wxWindow *parent, BoatSpeed &b)
    : BoatDialogBase(parent), boat(b), m_PlotScale(0)
{
}

BoatDialog::~BoatDialog()
{
}

void BoatDialog::OnMouseEventsPlot( wxMouseEvent& event )
{
    if(event.Leaving()) {
        m_stTrueWindAngle->SetLabel(_("N/A"));
        m_stTrueWindKnots->SetLabel(_("N/A"));
        m_stApparentWindAngle->SetLabel(_("N/A"));
        m_stApparentWindKnots->SetLabel(_("N/A"));
        m_stBoatAngle->SetLabel(_("N/A"));
        m_stBoatKnots->SetLabel(_("N/A"));
    }

    if(!m_PlotScale)
        return;

    wxPoint p = event.GetPosition();
    wxSize s = m_PlotWindow->GetSize();

    /* range + to - */
    double x = (double)p.x - s.x/2;
    double y = (double)p.y - s.y/2;

    /* range +- */
    x /= m_PlotScale;
    y /= m_PlotScale;

    switch(m_cPlotAxis->GetSelection()) {
    case 0: /* Boat */
    {
        double BA = rad2posdeg(atan2(x, -y)), VB = hypot(x, y);
        m_stBoatAngle->SetLabel(wxString::Format(_T("%03.0f"), BA));
        m_stBoatKnots->SetLabel(wxString::Format(_T("%.1f"), VB));

        double W = positive_degrees(-BA), VW = boat.TrueWindSpeed(0, VB, W, 30);

        m_stTrueWindAngle->SetLabel(wxString::Format(_T("%03.0f"), W));
        m_stTrueWindKnots->SetLabel(wxString::Format(_T("%.1f"), VW));

        double VA = BoatSpeed::VelocityApparentWind(VB, deg2rad(W), VW);
        double A = rad2posdeg(BoatSpeed::DirectionApparentWind(VA, VB, deg2rad(W), VW));

        m_stApparentWindAngle->SetLabel(wxString::Format(_T("%03.0f"), A));

        m_stApparentWindKnots->SetLabel(wxString::Format(_T("%.1f"), VA));

    } break;
    case 1: /* True Wind */
    {
        x *= m_PlotScale, y *= m_PlotScale;
        double W = rad2posdeg(atan2(x, -y)), VW = hypot(x, y);
        m_stTrueWindAngle->SetLabel(wxString::Format(_T("%03.0f"), W));
        m_stTrueWindKnots->SetLabel(wxString::Format(_T("%.1f"), VW));

        double BA, VB;
        boat.Speed(0, W, VW, BA, VB);

        m_stBoatAngle->SetLabel(wxString::Format(_T("%03.0f"), BA));
        m_stBoatKnots->SetLabel(wxString::Format(_T("%.1f"), VB));

        double VA = BoatSpeed::VelocityApparentWind(VB, W, VW);
        double A = BoatSpeed::DirectionApparentWind(VA, VB, W, VW);

        m_stApparentWindAngle->SetLabel(wxString::Format(_T("%03.0f"), A));
        m_stApparentWindKnots->SetLabel(wxString::Format(_T("%.1f"), VA));

    } break;
    case 2: /* Apparent Wind */
    {

    } break;
    }
}

void BoatDialog::OnPaintPlot(wxPaintEvent& event)
{
    wxWindow *window = dynamic_cast<wxWindow*>(event.GetEventObject());
    if(!window)
        return;

    wxPaintDC dc(window);
    dc.SetTextForeground(wxColour(1, 0, 0));
    dc.SetBackgroundMode(wxTRANSPARENT);

    int w, h;
    m_PlotWindow->GetSize( &w, &h);
    m_PlotScale = (w < h ? w : h)/1.8;
    double maxVB = 0;
    const double maxVW = 25;

    for(int VW = maxVW; VW > 0; VW -= 5) {
        wxPoint points[DEGREES / DEGREE_STEP];
        int W, i;
        for(W = 0, i=0; W<DEGREES; W += DEGREE_STEP, i++) {
            double BA, VB;
            boat.Speed(0, W, VW, BA, VB);

            if(VW == maxVW && VB > maxVB)
                maxVB = VB;

            switch(m_cPlotAxis->GetSelection()) {
            case 0: /* boat */
            {
                points[i] = wxPoint(1000*VB*sin(deg2rad(BA)), -1000*VB*cos(deg2rad(BA)));
            } break;
            case 1: /* true wind */
            {

            } break;
            case 2: /* apparent wind */
            {
            } break;
            }
        }

        if(VW == maxVW)
            m_PlotScale = m_PlotScale / (maxVB+1);

        for(W = 0, i=0; W<DEGREES; W += DEGREE_STEP, i++)
            points[i] = wxPoint(m_PlotScale * points[i].x / 1000, m_PlotScale * points[i].y / 1000);

        dc.DrawPolygon(i, points, w/2, h/2, 0);
    }
}

void BoatDialog::OnOpen ( wxCommandEvent& event )
{
    wxFileDialog openDialog( this, _( "Select CSV Polar" ), _("/home/sean/qtVlm/polar"), wxT ( "" ),
                             wxT ( "CSV files (*.CSV)|*.CSV;*.csv|All files (*.*)|*.*" ), wxFD_OPEN  );
    if( openDialog.ShowModal() == wxID_OK ) {
        BoatSpeedTable table;

        if(table.Open(openDialog.GetPath().ToAscii()))
            boat.SetSpeedsFromTable(table);
        else
        {
            wxMessageDialog md(this, _("Failed Loading boat polar"), _("OpenCPN Weather Routing Plugin"),
                               wxICON_INFORMATION | wxOK );
            md.ShowModal();
        }

        m_PlotWindow->Refresh();
    }
}

void BoatDialog::OnSave ( wxCommandEvent& event )
{
    wxFileDialog saveDialog( this, _( "Select CSV Polar" ), _("/home/sean/qtVlm/polar"), wxT ( "" ),
                             wxT ( "CSV files (*.CSV)|*.CSV;*.csv|All files (*.*)|*.*" ), wxFD_SAVE  );
    if( saveDialog.ShowModal() == wxID_OK ) {
        BoatSpeedTable table = boat.CreateTable();
        if(!table.Save(saveDialog.GetPath().ToAscii())) {
            wxMessageDialog md(this, _("Failed saving boat polar"), _("OpenCPN Weather Routing Plugin"),
                               wxICON_INFORMATION | wxOK );
            md.ShowModal();
        }
    }
}

void BoatDialog::OnOptimizeTacking ( wxCommandEvent& event )
{
    boat.OptimizeTackingSpeed();
    m_PlotWindow->Refresh();
}

void BoatDialog::Compute()
{
    boat.ComputeBoatSpeeds(m_sEta->GetValue() / 1000.0,
                           m_sKeelPressure->GetValue() / 100.0,
                           m_sKeelLift->GetValue() / 100.0);
}

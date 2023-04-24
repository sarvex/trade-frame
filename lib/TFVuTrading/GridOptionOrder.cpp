/************************************************************************
 * Copyright(c) 2023, One Unified. All rights reserved.                 *
 * email: info@oneunified.net                                           *
 *                                                                      *
 * This file is provided as is WITHOUT ANY WARRANTY                     *
 *  without even the implied warranty of                                *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                *
 *                                                                      *
 * This software may not be used nor distributed without proper license *
 * agreement.                                                           *
 *                                                                      *
 * See the file LICENSE.txt for redistribution information.             *
 ************************************************************************/

/*
 * File:    GridOptionOrder.cpp
 * Author:  raymond@burkholder.net
 * Project: TFVuTrading
 * Created: 2023/04/23 17:28:15
 */

#include "GridOptionOrder.hpp"
#include "GridOptionOrder_impl.hpp"

namespace ou { // One Unified
namespace tf { // TradeFrame

GridOptionOrder::GridOptionOrder(): wxGrid() {
  Init();
}

GridOptionOrder::GridOptionOrder(
  wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& sTitle )
: wxGrid(  )
{
  Init();
  Create(parent, id, pos, size, style, sTitle );
}

GridOptionOrder::~GridOptionOrder() {
}

void GridOptionOrder::Init() {
}

bool GridOptionOrder::Create(
  wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) {

  wxGrid::Create( parent, id, pos, size, style, name );
  m_pimpl = std::make_unique<GridOptionOrder_impl>( *this );

  CreateControls();

  return true;
}

void GridOptionOrder::CreateControls() {

  //Bind( wxEVT_CLOSE_WINDOW, &GridOptionDetails::OnClose, this );  // not called for child windows
  Bind( wxEVT_DESTROY, &GridOptionOrder::OnDestroy, this );

  //Bind( wxEVT_PAINT, &WinChartView::HandlePaint, this );
  //Bind( wxEVT_SIZE, &GridOptionDetails::HandleSize, this );

  //Bind( wxEVT_MOUSEWHEEL, &WinChartView::HandleMouseWheel, this );
  //Bind( wxEVT_ENTER_WINDOW, &WinChartView::HandleMouseEnter, this );
  //Bind( wxEVT_LEAVE_WINDOW, &WinChartView::HandleMouseLeave, this );

  //Bind( EVENT_DRAW_CHART, &WinChartView::HandleGuiDrawChart, this );

  // this GuiRefresh initialization should come after all else
  //m_timerGuiRefresh.SetOwner( this );
  //Bind( wxEVT_TIMER, &GridOptionDetails::HandleGuiRefresh, this, m_timerGuiRefresh.GetId() );
  //m_timerGuiRefresh.Start( 250 );

  m_pimpl->CreateControls();
}

void GridOptionOrder::OnDestroy( wxWindowDestroyEvent& event ) {

  //m_timerGuiRefresh.Stop();
  //Unbind( wxEVT_TIMER, &WinChartView::HandleGuiRefresh, this, m_timerGuiRefresh.GetId() );

  //m_pimpl->StopWatch();
  m_pimpl->DestroyControls();
  m_pimpl.reset();

  Unbind( wxEVT_DESTROY, &GridOptionOrder::OnDestroy, this );

  //Unbind( wxEVT_PAINT, &WinChartView::HandlePaint, this );
  //Unbind( wxEVT_SIZE, &GridOptionDetails::HandleSize, this );

  //Unbind( wxEVT_MOTION, &WinChartView::HandleMouse, this );
  //Unbind( wxEVT_MOUSEWHEEL, &WinChartView::HandleMouseWheel, this );
  //Unbind( wxEVT_ENTER_WINDOW, &WinChartView::HandleMouseEnter, this );
  //Unbind( wxEVT_LEAVE_WINDOW, &WinChartView::HandleMouseLeave, this );

  event.Skip();  // auto followed by Destroy();
}

} // namespace tf
} // namespace ou
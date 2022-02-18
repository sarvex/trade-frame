/************************************************************************
 * Copyright(c) 2022, One Unified. All rights reserved.                 *
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
 * File:    Strategy.cpp
 * Author:  raymond@burkholder.net
 * Project: AutoTrade
 * Created: February 14, 2022 10:59
 */

#include <OUCharting/ChartDataView.h>

#include <TFTrading/Watch.h>

#include "Config.h"
#include "Strategy.h"

using pWatch_t = ou::tf::Watch::pWatch_t;

Strategy::Strategy( ou::ChartDataView& cdv, const config::Options& options )
: ou::tf::DailyTradeTimeFrame<Strategy>()
, m_cdv( cdv )
, m_ceShortEntry( ou::ChartEntryShape::EShort, ou::Colour::Red )
, m_ceLongEntry( ou::ChartEntryShape::ELong, ou::Colour::Blue )
, m_ceShortFill( ou::ChartEntryShape::EFillShort, ou::Colour::Red )
, m_ceLongFill( ou::ChartEntryShape::EFillLong, ou::Colour::Blue )
, m_ceShortExit( ou::ChartEntryShape::EShortStop, ou::Colour::Red )
, m_ceLongExit( ou::ChartEntryShape::ELongStop, ou::Colour::Blue )
, m_bfQuotes01Sec( 1 )
, m_stateTrade( ETradeState::Init )
{

  assert( 0 < options.nPeriodWidth );

  m_nPeriodWidth = options.nPeriodWidth;
  m_vMAPeriods.push_back( options.nMA1Periods );
  m_vMAPeriods.push_back( options.nMA2Periods );
  m_vMAPeriods.push_back( options.nMA3Periods );

  assert( 3 == m_vMAPeriods.size() );
  for ( vMAPeriods_t::value_type value: m_vMAPeriods ) {
    assert( 0 < value );
  }

  m_ceQuoteAsk.SetColour( ou::Colour::Red );
  m_ceQuoteBid.SetColour( ou::Colour::Blue );
  m_ceTrade.SetColour( ou::Colour::DarkGreen );

  m_ceQuoteAsk.SetName( "Ask" );
  m_ceTrade.SetName( "Tick" );
  m_ceQuoteBid.SetName( "Bid" );

  m_ceVolume.SetName( "Volume" );

  m_ceProfitLoss.SetName( "P/L" );

  m_bfQuotes01Sec.SetOnBarComplete( MakeDelegate( this, &Strategy::HandleBarQuotes01Sec ) );
}

Strategy::~Strategy() {
  Clear();
}

void Strategy::SetupChart() {

  m_cdv.Add( EChartSlot::Price, &m_ceQuoteAsk );
  m_cdv.Add( EChartSlot::Price, &m_ceTrade );
  m_cdv.Add( EChartSlot::Price, &m_ceQuoteBid );

  m_cdv.Add( EChartSlot::Price, &m_ceLongEntry );
  m_cdv.Add( EChartSlot::Price, &m_ceLongFill );
  m_cdv.Add( EChartSlot::Price, &m_ceLongExit );
  m_cdv.Add( EChartSlot::Price, &m_ceShortEntry );
  m_cdv.Add( EChartSlot::Price, &m_ceShortFill );
  m_cdv.Add( EChartSlot::Price, &m_ceShortExit );

  m_cdv.Add( EChartSlot::Volume, &m_ceVolume );

  m_cdv.Add( EChartSlot::PL, &m_ceProfitLoss );

}

void Strategy::SetPosition( pPosition_t pPosition ) {

  assert( pPosition );

  Clear();

  m_pPosition = pPosition;
  pWatch_t pWatch = m_pPosition->GetWatch();

  m_cdv.SetNames( "Moving Average Strategy", pWatch->GetInstrument()->GetInstrumentName() );

  SetupChart();

  time_duration td = time_duration( 0, 0, m_nPeriodWidth );

  m_vMA.emplace_back( MA( pWatch->GetQuotes(), m_vMAPeriods[0], td, ou::Colour::Gold, "ma1" ) );
  m_vMA.emplace_back( MA( pWatch->GetQuotes(), m_vMAPeriods[1], td, ou::Colour::Coral, "ma2" ) );
  m_vMA.emplace_back( MA( pWatch->GetQuotes(), m_vMAPeriods[2], td, ou::Colour::Brown, "ma3" ) );

  for ( vMA_t::value_type& ma: m_vMA ) {
    ma.AddToView( m_cdv );
  }

  pWatch->OnQuote.Add( MakeDelegate( this, &Strategy::HandleQuote ) );
  pWatch->OnTrade.Add( MakeDelegate( this, &Strategy::HandleTrade ) );

}

void Strategy::Clear() {
  if  ( m_pPosition ) {
    pWatch_t pWatch = m_pPosition->GetWatch();
    pWatch->OnQuote.Remove( MakeDelegate( this, &Strategy::HandleQuote ) );
    pWatch->OnTrade.Remove( MakeDelegate( this, &Strategy::HandleTrade ) );
    m_cdv.Clear();
    m_vMA.clear();
    m_pPosition.reset();
  }
}

void Strategy::HandleQuote( const ou::tf::Quote& quote ) {
  // position has the quotes via the embedded watch
  // indicators are also attached to the embedded watch

  if ( !quote.IsValid() ) {
    return;
  }

  ptime dt( quote.DateTime() );

  m_ceQuoteAsk.Append( dt, quote.Ask() );
  m_ceQuoteBid.Append( dt, quote.Bid() );

  m_dblMid = quote.Midpoint();

  for ( vMA_t::value_type& ma: m_vMA ) {
    ma.Update( dt );
  }

  // feed the quote into DailyTradeTimeFrame for proper processing of the trading day
  m_bfQuotes01Sec.Add( dt, quote.Spread(), 1 ); // provides a 1 sec pulse for checking the alogorithm (ignores the spread)

}

void Strategy::HandleTrade( const ou::tf::Trade& trade ) {

  ptime dt( trade.DateTime() );

  m_ceTrade.Append( dt, trade.Price() );
  m_ceVolume.Append( dt, trade.Volume() );

}

void Strategy::HandleBarQuotes01Sec( const ou::tf::Bar& bar ) {

  double dblUnRealized, dblRealized, dblCommissionsPaid, dblTotal;

  m_pPosition->QueryStats( dblUnRealized, dblRealized, dblCommissionsPaid, dblTotal );
  m_ceProfitLoss.Append( bar.DateTime(), dblTotal );

  TimeTick( bar );
}

void Strategy::HandleRHTrading( const ou::tf::Bar& bar ) { // once a second
  // DailyTradeTimeFrame: Trading during regular active equity market hours
  // https://learnpriceaction.com/3-moving-average-crossover-strategy/

  double ma1 = m_vMA[0].m_dblPrice;
  double ma2 = m_vMA[1].m_dblPrice;
  double ma3 = m_vMA[2].m_dblPrice;

  switch ( m_stateTrade ) {
    case ETradeState::Search:
      // TODO: include the marketRule price difference here?
      if ( ( ma1 > ma3 ) && ( ma2 > ma3 ) && ( m_dblMid > ma1 ) ) {
        // enter long
        m_pOrder = m_pPosition->ConstructOrder( ou::tf::OrderType::Market, ou::tf::OrderSide::Buy, 100 );
        m_pOrder->OnOrderCancelled.Add( MakeDelegate( this, &Strategy::HandleOrderCancelled ) );
        m_pOrder->OnOrderFilled.Add( MakeDelegate( this, &Strategy::HandleOrderFilled ) );
        m_ceLongEntry.AddLabel( bar.DateTime(), m_dblMid, "Long Submit" );
        m_stateTrade = ETradeState::LongSubmitted;
        m_pPosition->PlaceOrder( m_pOrder );
      }
      else {
        if ( ( ma1 < ma3 ) && ( ma2 < ma3 ) && ( m_dblMid < ma1 ) ) {
          // enter short
          m_pOrder = m_pPosition->ConstructOrder( ou::tf::OrderType::Market, ou::tf::OrderSide::Sell, 100 );
          m_pOrder->OnOrderCancelled.Add( MakeDelegate( this, &Strategy::HandleOrderCancelled ) );
          m_pOrder->OnOrderFilled.Add( MakeDelegate( this, &Strategy::HandleOrderFilled ) );
          m_ceShortEntry.AddLabel( bar.DateTime(), m_dblMid, "Short Submit" );
          m_stateTrade = ETradeState::ShortSubmitted;
          m_pPosition->PlaceOrder( m_pOrder );
        }
      }
      break;
    case ETradeState::LongSubmitted:
      // wait for order to execute
      break;
    case ETradeState::LongExit:
      if ( ma1 < ma3 ) {
        // exit long
        m_pOrder = m_pPosition->ConstructOrder( ou::tf::OrderType::Market, ou::tf::OrderSide::Sell, 100 );
        m_pOrder->OnOrderCancelled.Add( MakeDelegate( this, &Strategy::HandleOrderCancelled ) );
        m_pOrder->OnOrderFilled.Add( MakeDelegate( this, &Strategy::HandleOrderFilled ) );
        m_ceLongExit.AddLabel( bar.DateTime(), m_dblMid, "Long Exit" );
        m_stateTrade = ETradeState::ExitSubmitted;
        m_pPosition->PlaceOrder( m_pOrder );
      }
      break;
    case ETradeState::ShortSubmitted:
      // wait for order to execute
      break;
    case ETradeState::ShortExit:
      if ( ma1 > ma3 ) {
        // exit short
        m_pOrder = m_pPosition->ConstructOrder( ou::tf::OrderType::Market, ou::tf::OrderSide::Buy, 100 );
        m_pOrder->OnOrderCancelled.Add( MakeDelegate( this, &Strategy::HandleOrderCancelled ) );
        m_pOrder->OnOrderFilled.Add( MakeDelegate( this, &Strategy::HandleOrderFilled ) );
        m_ceShortExit.AddLabel( bar.DateTime(), m_dblMid, "Short Exit" );
        m_stateTrade = ETradeState::ExitSubmitted;
        m_pPosition->PlaceOrder( m_pOrder );
      }
      break;
    case ETradeState::ExitSubmitted:
      // wait for order to execute
      break;
    case ETradeState::Done:
      // quiescent
      break;
    case ETradeState::Init:
      // market open statistics management here
      // will need to wait for ma to load & diverge (based upon width & period)
      m_stateTrade = ETradeState::Search;
      break;
  }
}

void Strategy::HandleOrderCancelled( const ou::tf::Order& ) {
  m_pOrder->OnOrderCancelled.Remove( MakeDelegate( this, &Strategy::HandleOrderCancelled ) );
  m_pOrder->OnOrderFilled.Remove( MakeDelegate( this, &Strategy::HandleOrderFilled ) );
  switch ( m_stateTrade ) {
    case ETradeState::ExitSubmitted:
      assert( false );  // TODO: need to figure out a plan to retry exit
      break;
    default:
      m_stateTrade = ETradeState::Search;
  }
  m_pOrder.reset();
}

void Strategy::HandleOrderFilled( const ou::tf::Order& order ) {
  m_pOrder->OnOrderCancelled.Remove( MakeDelegate( this, &Strategy::HandleOrderCancelled ) );
  m_pOrder->OnOrderFilled.Remove( MakeDelegate( this, &Strategy::HandleOrderFilled ) );
  switch ( m_stateTrade ) {
    case ETradeState::LongSubmitted:
      m_ceLongFill.AddLabel( order.GetDateTimeOrderFilled(), m_dblMid, "Long Fill" );
      m_stateTrade = ETradeState::LongExit;
      break;
    case ETradeState::ShortSubmitted:
      m_ceShortFill.AddLabel( order.GetDateTimeOrderFilled(), m_dblMid, "Short Fill" );
      m_stateTrade = ETradeState::ShortExit;
      break;
    case ETradeState::ExitSubmitted:
      m_stateTrade = ETradeState::Search;
      break;
    default:
      assert( false ); // TODO: unravel the state mess if we get here
  }
  m_pOrder.reset();
}

void Strategy::HandleCancel( boost::gregorian::date, boost::posix_time::time_duration ) { // one shot
  m_pPosition->CancelOrders();
}

void Strategy::HandleGoNeutral( boost::gregorian::date, boost::posix_time::time_duration ) { // one shot
  m_pPosition->ClosePosition();
}

void Strategy::SaveWatch( const std::string& sPrefix ) {
  m_pPosition->GetWatch()->SaveSeries( sPrefix );
}

void Strategy::CloseAndDone() {
  m_pPosition->ClosePosition();
  m_stateTrade = ETradeState::Done;
}
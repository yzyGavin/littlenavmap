/*****************************************************************************
* Copyright 2015-2019 Alexander Barthel alex@littlenavmap.org
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*****************************************************************************/

#include "mappainter/mappainterweather.h"

#include "common/symbolpainter.h"
#include "mapgui/mapscale.h"
#include "mapgui/maplayer.h"
#include "query/mapquery.h"
#include "util/paintercontextsaver.h"
#include "navapp.h"
#include "fs/weather/metar.h"
#include "weather/weatherreporter.h"

#include <marble/GeoPainter.h>
#include <marble/ViewportParams.h>

using namespace Marble;
using namespace atools::geo;
using namespace map;

MapPainterWeather::MapPainterWeather(MapPaintWidget* mapWidget, MapScale *mapScale)
  : MapPainter(mapWidget, mapScale)
{
}

MapPainterWeather::~MapPainterWeather()
{
}

void MapPainterWeather::render(PaintContext *context)
{
  bool drawWeather = context->objectDisplayTypes.testFlag(map::AIRPORT_WEATHER) &&
                     context->mapLayer->isAirportWeather() &&
                     context->viewContext == Marble::Still;

  if(!drawWeather)
    return;

  atools::util::PainterContextSaver saver(context->painter);
  Q_UNUSED(saver);

  // Get airports from cache/database for the bounding rectangle and add them to the map
  const GeoDataLatLonAltBox& curBox = context->viewport->viewLatLonAltBox();
  const QList<MapAirport> *airportCache = mapQuery->getAirports(curBox, context->mapLayer, context->lazyUpdate);

  if(airportCache == nullptr)
    return;

  // Collect all airports that are visible
  QList<PaintAirportType> visibleAirportWeather;
  for(const MapAirport& airport : *airportCache)
  {
    float x, y;
    bool hidden;
    bool visibleOnMap = wToS(airport.position, x, y, scale->getScreeenSizeForRect(airport.bounding), &hidden);

    if(!hidden && visibleOnMap)
      visibleAirportWeather.append({&airport, QPointF(x, y)});
  }

  // Sort by airport display order
  std::sort(visibleAirportWeather.begin(), visibleAirportWeather.end(), sortAirportFunction);

  for(const PaintAirportType& airportWeather: visibleAirportWeather)
  {
    float x = static_cast<float>(airportWeather.point.x());
    float y = static_cast<float>(airportWeather.point.y());
    drawAirportWeather(context,
                       NavApp::getWeatherReporter()->getAirportWeather(airportWeather.airport->ident,
                                                                       airportWeather.airport->position,
                                                                       context->weatherSource), x, y);
  }
}

void MapPainterWeather::drawAirportWeather(PaintContext *context,
                                           const atools::fs::weather::Metar& metar, float x, float y)
{
  float size = context->sz(context->symbolSizeAirport, context->mapLayerEffective->getAirportSymbolSize());
  bool windBarbs = context->mapLayer->isAirportWeatherDetails();

  symbolPainter->drawAirportWeather(context->painter, metar, x - size * 4.f / 5.f, y - size * 4.f / 5.f, size,
                                    true /* Wind pointer*/, windBarbs, context->drawFast);
}

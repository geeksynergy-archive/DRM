/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2006
 *
 * Author(s):
 *	Julian Cable
 *
 * Description:
 *	ETSI DAB/DRM Electronic Programme Guide XML Decompressor
 *
 *
 ******************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
\******************************************************************************/

#include <string>
#include <map>
#include <iostream>
#include <sstream>
#include "epgdec.h"
#include "../datadecoding/DABMOT.h"
#include "../util/Utilities.h"

static QDomElement element(QDomDocument& doc, const tag_length_value& tlv);

void
CEPGDecoder::decode (const vector<_BYTE>& vecData)
{
    /* clear the doc, allowing re-use */
    doc.setContent (QString (""));
    tag_length_value tlv(&vecData[0]);
    if(tlv.is_epg()) {
      doc.appendChild (element(doc, tlv));
    }
}

typedef enum { nu_attr, enum_attr, string_attr, u16_attr, u24_attr, datetime_attr,
  duration_attr, sid_attr, genre_href_attr, bitrate_attr} enum_attr_t;

typedef struct
{
    const char *name;
    const char **vals;
    enum_attr_t decode;
} dectab;

static char token_list[20][255];

static uint32_t default_content_id;

static const char *enums0[] = { (char*)2, "DAB", "DRM" };
static const char *enums1[] = { (char*)9, 0, "series",
    "show",
    "programConcept",
    "magazine",
    "programCompilation",
    "otherCollection",
    "otherChoice",
    "topic"
};
static const char *enums2[] = { (char*)3, "URL", "DAB", "DRM" };
static const char *enums3[] = { (char*)4, "identical", "more", "less", "similar" };
static const char *enums4[] = { (char*)2, "primary", "alternative" };
static const char *enums5[] = { (char*)7, "audio", "DLS",
    "MOTSlideshow", "MOTBWS", "TPEG", "DGPS", "proprietary"
};
static const char *enums6[] = { (char*)2, "primary", "secondary" };
static const char *enums7[] = { (char*)2, "none", "unspecified" };
static const char *enums8[] = { (char*)2, "on-air", "off-air" };
static const char *enums9[] = { (char*)2, "no", "yes" };
static const char *enums10[] =
    { (char*)4, 0, "logo_unrestricted", "logo_mono_square",
    "logo_colour_square", "logo_mono_rectangle"
};

static const char *enums11[] = { (char*)3, "main", "secondary", "other" };

static const char *classificationScheme[] = {
    0,
    "IntentionCS",
    "FormatCS",
    "ContentCS",
    "IntendedAudienceCS",
    "OriginationCS",
    "ContentalertCS",
    "MediaTypeCS",
    "AtmosphereCS",
    0, 0, 0, 0, 0, 0, 0
};


static dectab attribute_tags_epg[] = {
    {"system", enums0, enum_attr},
    {"id", 0, string_attr}
};

static dectab attribute_tags_sch[] = {
    {"version", 0, u16_attr},
    {"creationTime", 0, datetime_attr},
    {"originator", 0, string_attr}
};

static dectab attribute_tags_si[] = {
    {"version", 0, u16_attr},
    {"creationTime", 0, datetime_attr},
    {"originator", 0, string_attr},
    {"serviceProvider", 0, string_attr},
    {"system", enums0, enum_attr}
};

static dectab attribute_tags2[] = {
    {"shortId", 0, u24_attr},
    {"version", 0, u16_attr},
    {"type", enums1, enum_attr},
    {"numOfItems", 0, u16_attr},
};
static dectab attribute_tags_scope[] = {
    {"startTime", 0, datetime_attr},
    {"stopTime", 0, datetime_attr}
};
static dectab attribute_tags4[] = {
    {"protocol", enums2, enum_attr},
    {"type", enums3, enum_attr},
    {"url", 0, string_attr},
};
//static dectab attribute_tags5[] = {
//    {"id", 0, string_attr},
//    {"version", 0, u16_attr}
//};
static dectab attribute_tags6[] = {
    {"type", enums4, enum_attr},
    {"kHz", 0, u24_attr}
};
static dectab attribute_tags7[] = {
    {"version", 0, u16_attr},
    {"format", enums5, enum_attr},
    {"Not used", 0, nu_attr},
    {"bitrate", 0, bitrate_attr}
};
static dectab attribute_tags8[] = {
    {"id", 0, string_attr},
    {"type", enums6, enum_attr}
};
static dectab attribute_tags_name[] = {
    {"xml:lang", 0, string_attr}
};
static dectab attribute_tags10[] = {
    {"mimeValue", 0, string_attr},
    {"xml:lang", 0, string_attr},
    {"url", 0, string_attr},
    {"type", enums10, enum_attr},
    {"width", 0, u16_attr},
    {"height", 0, u16_attr}
};
static dectab attribute_tags11[] = {
    {"time", 0, datetime_attr},
    {"duration", 0, duration_attr},
    {"actualTime", 0, datetime_attr},
    {"actualDuration", 0, duration_attr}
};
static dectab attribute_tags12[] = {
    {"id", 0, sid_attr},
    {"trigger", 0, u16_attr}
};
static dectab attribute_tags13[] = {
    {"id", 0, string_attr},
    {"shortId", 0, u24_attr},
    {"index", 0, u16_attr}
};
static dectab attribute_tags14[] = {
    {"url", 0, string_attr},
    {"mimeValue", 0, string_attr},
    {"xml:lang", 0, string_attr},
    {"description", 0, string_attr},
    {"expiryTime", 0, datetime_attr}
};
static dectab attribute_tags15[] = {
    {"id", 0, string_attr},
    {"shortId", 0, u24_attr},
    {"version", 0, u16_attr},
    {"recommendation", enums9, enum_attr},
    {"broadcast", enums8, enum_attr},
    {"Not used", 0, nu_attr},
    {"xml:lang", 0, string_attr},
    {"bitrate", 0, string_attr}
};
static dectab attribute_tags_genre[] = {
    {"href", 0, genre_href_attr},
    {"type", enums11, enum_attr}
};

static dectab attribute_tags18[] = {
    {"type", enums7, enum_attr}
};

struct eltab_t {
  const char * element_name;
  dectab* tags;
  size_t size;
};

static eltab_t element_tables[] = {
    { "", 0, 0 },
    { "", 0, 0 },
    { "epg", attribute_tags_epg, sizeof (attribute_tags_epg) / sizeof (dectab) },
    { "serviceInformation", attribute_tags_si, sizeof (attribute_tags_si) / sizeof (dectab) },
    { "", 0, 0 },
    { "", 0, 0 },
    { "", 0, 0 },
    { "", 0, 0 },
    { "", 0, 0 },
    { "", 0, 0 },
    { "", 0, 0 },
    { "", 0, 0 },
    { "", 0, 0 },
    { "", 0, 0 },
    { "", 0, 0 },
    { "", 0, 0 },
    { "shortName", attribute_tags_name, sizeof (attribute_tags_name) / sizeof (dectab) },
    { "mediumName", attribute_tags_name, sizeof (attribute_tags_name) / sizeof (dectab) },
    { "longName", attribute_tags_name, sizeof (attribute_tags_name) / sizeof (dectab) },
    { "mediaDescription", 0, 0 },
    { "genre", attribute_tags_genre, sizeof (attribute_tags_genre) / sizeof (dectab) },
    { "CA", attribute_tags18, sizeof (attribute_tags18) / sizeof (dectab) },
    { "keywords", attribute_tags_name, sizeof (attribute_tags_name) / sizeof (dectab) },
    { "memberOf", attribute_tags13, sizeof (attribute_tags13) / sizeof (dectab) },
    { "link", attribute_tags14, sizeof (attribute_tags14) / sizeof (dectab) },
    { "location", 0, 0 },
    { "shortDescription", attribute_tags_name, sizeof (attribute_tags_name) / sizeof (dectab) },
    { "longDescription", attribute_tags_name, sizeof (attribute_tags_name) / sizeof (dectab) },
    { "programme", attribute_tags15, sizeof (attribute_tags15) / sizeof (dectab) },
    { "", 0, 0 },
    { "", 0, 0 },
    { "", 0, 0 },
    { "programmeGroups", attribute_tags_sch, sizeof (attribute_tags_sch) / sizeof (dectab) },
    { "schedule", attribute_tags_sch, sizeof (attribute_tags_sch) / sizeof (dectab) },
    { "alternateSource", attribute_tags4, sizeof (attribute_tags4) / sizeof (dectab) },
    { "programmeGroup", attribute_tags2, sizeof (attribute_tags2) / sizeof (dectab) },
    { "scope", attribute_tags_scope, sizeof (attribute_tags_scope) / sizeof (dectab) },
    { "serviceScope", attribute_tags12, sizeof (attribute_tags12) / sizeof (dectab) },
    { "ensemble", attribute_tags6, sizeof (attribute_tags6) / sizeof (dectab) },
    { "frequency", attribute_tags7, sizeof (attribute_tags7) / sizeof (dectab) },
    { "service", attribute_tags8, sizeof (attribute_tags8) / sizeof (dectab) },
    { "serviceID", attribute_tags6, sizeof (attribute_tags6) / sizeof (dectab) },
    { "epgLanguage", attribute_tags_name, sizeof (attribute_tags_name) / sizeof (dectab) },
    { "multimedia", attribute_tags10, sizeof (attribute_tags10) / sizeof (dectab) },
    { "time", attribute_tags11, sizeof (attribute_tags11) / sizeof (dectab) },
    { "bearer", attribute_tags12, sizeof (attribute_tags12) / sizeof (dectab) },
    { "programmeEvent", attribute_tags15, sizeof (attribute_tags15) / sizeof (dectab) },
    { "relativeTime", attribute_tags11, sizeof (attribute_tags11) / sizeof (dectab) },
    { "simulcast", attribute_tags_epg, sizeof (attribute_tags_epg) / sizeof (dectab) }
};




string decode_string (const _BYTE * p, size_t len);
const string element_name (_BYTE tag);
static void attribute(map<string,string>& out, _BYTE element_tag, tag_length_value& tlv);
static void string_token_table(const tag_length_value& tlv);

uint16_t
get_uint16 (const _BYTE* p)
{
    uint16_t h = p[0], l = p[1];
    return ((h << 8) | l);
}

uint32_t
get_uint24 (const _BYTE* p)
{
    uint32_t h = p[0], m = p[1], l = p[2];
    return ((((h << 8) | m) << 8) | l);
}

tag_length_value::tag_length_value(const _BYTE* q)
{
  _BYTE* p=const_cast<_BYTE*>(q);
  tag = *p++;
  length = *p++;
  if (length == 0xFE) {
     length = get_uint16(p);
     p+=2;
  }
  else if (length == 0xFF) {
     length = get_uint24(p);
     p+=3;
  }
  value = p;
}

static QDomElement
element(QDomDocument& doc, const tag_length_value& tlv)
{
  QString name (element_tables[tlv.tag].element_name);
  QDomElement e = doc.createElement (name);
  map<string,string> attr;
  _BYTE* end = tlv.value+tlv.length;
  dectab* at = element_tables[tlv.tag].tags;
  /* set default attributes */
  for(size_t k=0; k<element_tables[tlv.tag].size; k++) {
    if(at[k].decode == enum_attr)
      attr[at[k].name] = at[k].vals[1];
  }
  tag_length_value a(tlv.value);
  while(a.is_attribute())
  {
      attribute(attr, tlv.tag, a);
      _BYTE* p = a.value+a.length;
      if(p>=end)
        break;
      tag_length_value b(p);
      a = b;
  }
  for(map<string,string>::iterator i = attr.begin(); i != attr.end(); i++)
    e.setAttribute (QString(i->first.c_str()), QString().fromUtf8(i->second.c_str()));
  _BYTE* p = a.value;
  while(p<end)
  {
      if(a.is_string_token_table() && !tlv.is_child_element())
          string_token_table(a);
      else if(a.is_default_id() && !tlv.is_child_element()) {
	      default_content_id = get_uint24(p);
	      p+=3;
      }
      else if(a.is_child_element()) {
		e.appendChild (element(doc, a));
	}
      else if(a.is_cdata()) {
          string value = decode_string(a.value, a.length);
	  QDomText t = doc.createTextNode (QString ().fromUtf8 (value.c_str()));
	  e.appendChild (t);
      }
      p = a.value+a.length;
      if(p>=end)
        break;
      tag_length_value b(p);
      a = b;
  }
  return e;
}

static string
decode_genre_href (const _BYTE* p, size_t len)
{
    int cs = p[0] & 0xff;
    if(cs < 1 || cs > 8)
	    return "";
    stringstream out;
    out << "urn:tva:metadata:cs:" << classificationScheme[cs] << ":2005:";
    switch (len)
      {
      case 2:
	  out << cs << '.' << int(p[1]);
	  break;
      case 3:
	  out << cs << '.' << int(p[1]) << '.' << int(p[2]);
	  break;
      case 4:
	  out << cs << '.' << int(p[1]) << '.' << int(p[2]) << '.' << int(p[3]);
	  break;
      }
    return out.str();
}

string
decode_string (const _BYTE* p, size_t len)
{
    size_t i;
    string out;
    for (i = 0; i < len; i++)
    {
	  char c = p[i];
	  if (1 <= c && c <= 19)
	      if (c == 0x9 || c == 0xa || c == 0xd)
		      out += c;
	      else
		      out += token_list[p[i]];
	  else
	      out += c;
    }
    return out;
}

static string
decode_uint16 (const _BYTE* p)
{
    stringstream out;
    out << get_uint16(p);
    return out.str();
}

static string
decode_uint24 (const _BYTE* p)
{
    stringstream out;
    out << int(get_uint24(p));
    return out.str();
}

static string
decode_sid (const _BYTE* p)
{
    stringstream out;
    out << hex << int(p[0]) << '.' << int(p[1]) << '.' << int(p[2]);
    return out.str();
}

static string
decode_dateandtime(const _BYTE* p)
{
    uint32_t mjd;
    uint32_t h = p[0], m = p[1], l = p[2];
    uint16_t n, year;
    int month, day;
    int hours, minutes, seconds = 0;
    int utc_flag, lto_flag, sign = 0, lto = 0;
    mjd = (((((h << 8) | m) << 8) | l) >> 6) & 0x1ffff;
    lto_flag = p[2] & 0x10;
    utc_flag = p[2] & 0x08;
    n = (p[2] << 8) | p[3];
    hours = (n >> 6) & 0x1f;
    minutes = n & 0x3f;
    n = 4;
    if (utc_flag)
    {
    	seconds = p[n] >> 2;
    	n += 2;
    }
    //cerr << mjd << " " << hours << ":" << minutes;
    stringstream out;
    string tz = "Z";
    if (lto_flag)
    {
    	sign = p[n] & 0x20;
    	lto = p[n] & 0x3f;
    	int mins = 60*hours+minutes;
	//cerr << " lto: " << sign << " " << lto;
    	if(sign)
    	{
	    mins -= 30*lto;
	    if(mins<0)
	    {
		mjd--;
		mins += 24*60;
	    }
    	}
    	else
    	{
	    mins += 30*lto;
	    if(mins>(24*60))
	    {
		mjd++;
		mins -= 24*60;
	    }
    	}
	hours = mins / 60;
	minutes = mins % 60;
	stringstream tzs;
	tzs << (sign ? '-' : '+');
	int ltoh = lto/2;
	if(ltoh<10) tzs << '0';
	tzs << ltoh << ':';
	int ltom = (30 * lto) % 30;
	if(ltom<10) tzs << '0';
	tzs << ltom;
	tz = tzs.str();
    }
    if(hours>=24)
    {
	      hours -= 24;
	      mjd++;
    }
    CModJulDate ModJulDate(mjd);
    year = ModJulDate.GetYear();
    month = ModJulDate.GetMonth();
    day = ModJulDate.GetDay();
    out << year << '-';
    if(month<10) out << '0';
    out << int(month) << '-';
    if(day<10) out << '0';
    out << int(day) << 'T';
    if(hours<10) out << '0';
    out << hours << ':';
    if(minutes<10) out << '0';
    out << minutes << ':';
    if(seconds<10) out << '0';
    out << seconds << tz;
    //cerr << " -> " << out.str() << endl;
    return out.str();
}

static string
decode_duration (const _BYTE* p)
{
    uint16_t hours, minutes, seconds;
    seconds = get_uint16(p);
    minutes = seconds / 60;
    seconds = seconds % 60;
    hours = minutes / 60;
    minutes = minutes % 60;
    stringstream out;
    out << "PT";
    if(hours>0)
	out << hours << 'H';
    if(minutes>0)
	out <<  minutes << 'M';
    if(seconds>0)
	out << seconds << 'S';
    return out.str();
}

static string
decode_bitrate (const _BYTE* p)
{
    stringstream out;
    uint16_t n = get_uint16(p);
    out << float(n) / 0.1f;
    return out.str();
}

static string
decode_attribute_name(const dectab& tab)
{
	if (tab.name == NULL)
	  {
	      return "unknown";
	  }
	if (strlen (tab.name) > 64) /* some reasonably big number */
	  {
	      return "too long";
	  }
     return tab.name;
}

static string
decode_attribute_value (enum_attr_t format, const _BYTE* p, size_t len)
{
    switch(format) {
    case nu_attr:
         return "undefined";
         break;
    case enum_attr:
         return "undecoded enum";
         break;
    case string_attr:
         return decode_string(p, len);
         break;
    case u16_attr:
         return decode_uint16(p);
         break;
    case u24_attr:
         return decode_uint24(p);
         break;
    case datetime_attr:
         return decode_dateandtime(p);
         break;
    case duration_attr:
         return decode_duration(p);
         break;
    case sid_attr:
         return decode_sid(p);
         break;
    case genre_href_attr:
         return decode_genre_href(p, len);
         break;
    case bitrate_attr:
         return decode_bitrate(p);
         break;
	default:
		 return "";
		 break;
    }
}

static void attribute(map<string,string>& out, _BYTE element_tag, tag_length_value& tlv)
{
  size_t el = size_t(element_tag);
  size_t e = sizeof (element_tables) / sizeof (eltab_t);
  if (el >= e)
    {
      cerr << "illegal element id" << int(el) << endl;
      return;
    }
  eltab_t a = element_tables[el];
  size_t attr = tlv.tag&0x0f;
  size_t n = a.size;
  if (attr >= n) {
        cerr << "out of range attribute id " << attr << " for element id " << int(el) << endl;
  } else {
    dectab tab = a.tags[attr];
    string name = decode_attribute_name(tab);
    string value;
    if(tab.decode == enum_attr) {
		/* needed for 64 bit compatibility */
		ptrdiff_t index = tlv.value[0];
		ptrdiff_t num_vals = (tab.vals[0] - (const char*)0);
		if(index<=num_vals && index>0)
			value = tab.vals[index];
		else
			value = "out of range";
    } else {
      value = decode_attribute_value(tab.decode, tlv.value, tlv.length);
    }
    out[name] = value;
  }
}

static void string_token_table(const tag_length_value& tlv)
{
	  size_t i = 0;
	  _BYTE* p = tlv.value;
	  for (i = 0; i < 20; i++)
	      token_list[i][0] = 0;
	  for (i = 0; i < tlv.length;)
	    {
		_BYTE tok = p[i++];
		size_t tlen = p[i++];
		memcpy (token_list[tok], &p[i], tlen);
		token_list[tok][tlen] = 0;
		i += tlen;
	    }
}

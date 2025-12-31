#include "luaharfbuzz.h"
#include "tex/texmathparams.h"

#ifdef LuajitTeX
static int lua_absindex (lua_State *L, int i) {
  if (i < 0 && i > LUA_REGISTRYINDEX)
    i += lua_gettop(L) + 1;
  return i;
}
static int lua_geti (lua_State *L, int index, lua_Integer i) {
  index = lua_absindex(L, index);
  lua_pushinteger(L, i);
  lua_gettable(L, index);
  return lua_type(L, -1);
}
#endif

static int shape_full (lua_State *L) {
  Font *font = (Font *)luaL_checkudata(L, 1, "harfbuzz.Font");
  Buffer *buf = (Buffer *)luaL_checkudata(L, 2, "harfbuzz.Buffer");
  unsigned int i;
  luaL_checktype(L, 3, LUA_TTABLE);
  luaL_checktype(L, 4, LUA_TTABLE);

  unsigned int num_features = lua_rawlen(L, 3);
  Feature *features = (Feature *) malloc (num_features * sizeof(hb_feature_t));

  for (i = 0; i != num_features; ++i) {
    lua_geti(L, 3, i + 1);
    Feature* f = (Feature *)luaL_checkudata(L, -1, "harfbuzz.Feature");
    features[i] = *f;
    lua_pop(L, 1);
  }

  const char **shapers = NULL;
  size_t num_shapers = lua_rawlen(L, 4);
  if (num_shapers) {
    shapers = (const char**) calloc (num_shapers + 1, sizeof(char*));
    for (i = 0; i != num_shapers; ++i) {
      lua_geti(L, 4, i + 1);
      shapers[i] = luaL_checkstring(L, -1);
      lua_pop(L, 1);
    }
  }

  // Shape text
  lua_pushboolean(L, hb_shape_full(*font, *buf, features, num_features, shapers));

  free(features);
  free(shapers);

  return 1;
}

static int version (lua_State *L) {
  lua_pushstring(L, hb_version_string());
  return 1;
}

static int list_shapers (lua_State *L) {
  const char **shaper_list = hb_shape_list_shapers ();
  int i = 0;

  for (; *shaper_list; shaper_list++) {
    i++;
    lua_pushstring(L, *shaper_list);
  }
  return i;
}

static const struct luaL_Reg lib_table [] = {
  {"shape_full", shape_full},
  {"version", version},
  {"shapers", list_shapers},
  {NULL, NULL}
};

static const struct { int id; const char* name; } ot_math_constants[] = {
{HB_OT_MATH_CONSTANT_SCRIPT_PERCENT_SCALE_DOWN, "scriptPercentScaleDown"},
{HB_OT_MATH_CONSTANT_SCRIPT_SCRIPT_PERCENT_SCALE_DOWN, "scriptScriptPercentScaleDown"},
{HB_OT_MATH_CONSTANT_DELIMITED_SUB_FORMULA_MIN_HEIGHT, "delimitedSubFormulaMinHeight"},
{HB_OT_MATH_CONSTANT_DISPLAY_OPERATOR_MIN_HEIGHT, "displayOperatorMinHeight"},
{HB_OT_MATH_CONSTANT_MATH_LEADING, "mathLeading"},
{HB_OT_MATH_CONSTANT_AXIS_HEIGHT, "axisHeight"},
{HB_OT_MATH_CONSTANT_ACCENT_BASE_HEIGHT, "accentBaseHeight"},
{HB_OT_MATH_CONSTANT_FLATTENED_ACCENT_BASE_HEIGHT, "flattenedAccentBaseHeight"},
{HB_OT_MATH_CONSTANT_SUBSCRIPT_SHIFT_DOWN, "subscriptShiftDown"},
{HB_OT_MATH_CONSTANT_SUBSCRIPT_TOP_MAX, "subscriptTopMax"},
{HB_OT_MATH_CONSTANT_SUBSCRIPT_BASELINE_DROP_MIN, "subscriptBaselineDropMin"},
{HB_OT_MATH_CONSTANT_SUPERSCRIPT_SHIFT_UP, "superscriptShiftUp"},
{HB_OT_MATH_CONSTANT_SUPERSCRIPT_SHIFT_UP_CRAMPED, "superscriptShiftUpCramped"},
{HB_OT_MATH_CONSTANT_SUPERSCRIPT_BOTTOM_MIN, "superscriptBottomMin"},
{HB_OT_MATH_CONSTANT_SUPERSCRIPT_BASELINE_DROP_MAX, "superscriptBaselineDropMax"},
{HB_OT_MATH_CONSTANT_SUB_SUPERSCRIPT_GAP_MIN, "subSuperscriptGapMin"},
{HB_OT_MATH_CONSTANT_SUPERSCRIPT_BOTTOM_MAX_WITH_SUBSCRIPT, "superscriptBottomMaxWithSubscript"},
{HB_OT_MATH_CONSTANT_SPACE_AFTER_SCRIPT, "spaceAfterScript"},
{HB_OT_MATH_CONSTANT_UPPER_LIMIT_GAP_MIN, "upperLimitGapMin"},
{HB_OT_MATH_CONSTANT_UPPER_LIMIT_BASELINE_RISE_MIN, "upperLimitBaselineRiseMin"},
{HB_OT_MATH_CONSTANT_LOWER_LIMIT_GAP_MIN, "lowerLimitGapMin"},
{HB_OT_MATH_CONSTANT_LOWER_LIMIT_BASELINE_DROP_MIN, "lowerLimitBaselineDropMin"},
{HB_OT_MATH_CONSTANT_STACK_TOP_SHIFT_UP, "stackTopShiftUp"},
{HB_OT_MATH_CONSTANT_STACK_TOP_DISPLAY_STYLE_SHIFT_UP, "stackTopDisplayStyleShiftUp"},
{HB_OT_MATH_CONSTANT_STACK_BOTTOM_SHIFT_DOWN, "stackBottomShiftDown"},
{HB_OT_MATH_CONSTANT_STACK_BOTTOM_DISPLAY_STYLE_SHIFT_DOWN, "stackBottomDisplayStyleShiftDown"},
{HB_OT_MATH_CONSTANT_STACK_GAP_MIN, "stackGapMin"},
{HB_OT_MATH_CONSTANT_STACK_DISPLAY_STYLE_GAP_MIN, "stackDisplayStyleGapMin"},
{HB_OT_MATH_CONSTANT_STRETCH_STACK_TOP_SHIFT_UP, "stretchStackTopShiftUp"},
{HB_OT_MATH_CONSTANT_STRETCH_STACK_BOTTOM_SHIFT_DOWN, "stretchStackBottomShiftDown"},
{HB_OT_MATH_CONSTANT_STRETCH_STACK_GAP_ABOVE_MIN, "stretchStackGapAboveMin"},
{HB_OT_MATH_CONSTANT_STRETCH_STACK_GAP_BELOW_MIN, "stretchStackGapBelowMin"},
{HB_OT_MATH_CONSTANT_FRACTION_NUMERATOR_SHIFT_UP, "fractionNumeratorShiftUp"},
{HB_OT_MATH_CONSTANT_FRACTION_NUMERATOR_DISPLAY_STYLE_SHIFT_UP, "fractionNumeratorDisplayStyleShiftUp"},
{HB_OT_MATH_CONSTANT_FRACTION_DENOMINATOR_SHIFT_DOWN, "fractionDenominatorShiftDown"},
{HB_OT_MATH_CONSTANT_FRACTION_DENOMINATOR_DISPLAY_STYLE_SHIFT_DOWN, "fractionDenominatorDisplayStyleShiftDown"},
{HB_OT_MATH_CONSTANT_FRACTION_NUMERATOR_GAP_MIN, "fractionNumeratorGapMin"},
{HB_OT_MATH_CONSTANT_FRACTION_NUM_DISPLAY_STYLE_GAP_MIN, "fractionNumDisplayStyleGapMin"},
{HB_OT_MATH_CONSTANT_FRACTION_RULE_THICKNESS, "fractionRuleThickness"},
{HB_OT_MATH_CONSTANT_FRACTION_DENOMINATOR_GAP_MIN, "fractionDenominatorGapMin"},
{HB_OT_MATH_CONSTANT_FRACTION_DENOM_DISPLAY_STYLE_GAP_MIN, "fractionDenomDisplayStyleGapMin"},
{HB_OT_MATH_CONSTANT_SKEWED_FRACTION_HORIZONTAL_GAP, "skewedFractionHorizontalGap"},
{HB_OT_MATH_CONSTANT_SKEWED_FRACTION_VERTICAL_GAP, "skewedFractionVerticalGap"},
{HB_OT_MATH_CONSTANT_OVERBAR_VERTICAL_GAP, "overbarVerticalGap"},
{HB_OT_MATH_CONSTANT_OVERBAR_RULE_THICKNESS, "overbarRuleThickness"},
{HB_OT_MATH_CONSTANT_OVERBAR_EXTRA_ASCENDER, "overbarExtraAscender"},
{HB_OT_MATH_CONSTANT_UNDERBAR_VERTICAL_GAP, "underbarVerticalGap"},
{HB_OT_MATH_CONSTANT_UNDERBAR_RULE_THICKNESS, "underbarRuleThickness"},
{HB_OT_MATH_CONSTANT_UNDERBAR_EXTRA_DESCENDER, "underbarExtraDescender"},
{HB_OT_MATH_CONSTANT_RADICAL_VERTICAL_GAP, "radicalVerticalGap"},
{HB_OT_MATH_CONSTANT_RADICAL_DISPLAY_STYLE_VERTICAL_GAP, "radicalDisplayStyleVerticalGap"},
{HB_OT_MATH_CONSTANT_RADICAL_RULE_THICKNESS, "radicalRuleThickness"},
{HB_OT_MATH_CONSTANT_RADICAL_EXTRA_ASCENDER, "radicalExtraAscender"},
{HB_OT_MATH_CONSTANT_RADICAL_KERN_BEFORE_DEGREE, "radicalKernBeforeDegree"},
{HB_OT_MATH_CONSTANT_RADICAL_KERN_AFTER_DEGREE, "radicalKernAfterDegree"},
{HB_OT_MATH_CONSTANT_RADICAL_DEGREE_BOTTOM_RAISE_PERCENT, "radicalDegreeBottomRaisePercent"},
{0,0}
};

int luaopen_luaharfbuzz (lua_State *L) {
  lua_newtable(L);

  register_blob(L);
  lua_setfield(L, -2, "Blob");

  register_face(L);
  lua_setfield(L, -2, "Face");

  register_font(L);
  lua_setfield(L, -2, "Font");

  register_buffer(L);
  lua_setfield(L, -2, "Buffer");

  register_feature(L);
  lua_setfield(L, -2, "Feature");

  register_tag(L);
  lua_setfield(L, -2, "Tag");

  register_script(L);
  lua_setfield(L, -2, "Script");

  register_direction(L);
  lua_setfield(L, -2, "Direction");

  register_language(L);
  lua_setfield(L, -2, "Language");

  register_variation(L);
  lua_setfield(L, -2, "Variation");

  register_ot(L);
  lua_setfield(L, -2, "ot");

  register_unicode(L);
  lua_setfield(L, -2, "unicode");

  lua_newtable(L);
  const int tbl = lua_gettop(L);
  for( int i = 1; i < MATH_param_last; i++ )
  {
      lua_pushstring(L, MATH_param_names[i]);
      lua_pushinteger(L, i);
      lua_rawset(L, tbl);
  }
  lua_setfield(L, -2,"ot_math_constants" );

#ifdef LuajitTeX
  luaL_register(L,NULL, lib_table);
  /**/
  lua_pushvalue(L, -1);
  lua_setglobal(L,"luaharfbuzz");
  /**/
  lua_getglobal(L, "package");
  lua_getfield(L, -1, "loaded"); 
  lua_remove(L, -2);
  lua_pushvalue(L, -2);
  lua_setfield(L, -2, "luaharfbuzz"); 
  /**/
#else
  luaL_setfuncs(L, lib_table, 0);
#endif

  return 1;
}


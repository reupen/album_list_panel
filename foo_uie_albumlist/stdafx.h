#pragma once
#define OEMRESOURCE
#define NOMINMAX

#include <gsl/gsl>
#include <fmt/format.h>
#include <fmt/xchar.h>

#include <algorithm>
#include <optional>
#include <ranges>
#include <string_view>
#include <unordered_set>
#include <vector>

#include <ppl.h>
#include <concurrent_vector.h>

#include "../mmh/stdafx.h"
#include "../ui_helpers/stdafx.h"
#include "../fbh/stdafx.h"
#include "../columns_ui-sdk/ui_extension.h"

#include "../foobar2000/SDK/foobar2000.h"
#include "resource.h"
#include <commctrl.h>
#include <windowsx.h>

using namespace std::literals;
using namespace mmh::literals::pcc;

#include "node.h"
#include "config.h"
#include "main.h"
#include "tfhook.h"
#include "prefs.h"

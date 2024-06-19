#include <cstdint>
#include <algorithm>
#include <vector>
#include <numeric>
#include <cmath>
#include <numbers>
#include <bit>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

using byte = uint8_t;
#include <exedit.hpp>

////////////////////////////////
// 主要情報源の変数アドレス．
////////////////////////////////
constinit struct ExEdit092 {
	void init(AviUtl::FilterPlugin* efp)
	{
		if (fp == nullptr)
			init_pointers(efp);
	}
	AviUtl::FilterPlugin* fp = nullptr;

	// called at: exedit_base + 0x1c1ea.
	void*(*get_or_create_cache)(ExEdit::ObjectFilterIndex ofi, int w, int h, int bitcount, int v_func_id, int* old_cache_exists);

	// 0x04a7e0
	void(*update_any_exdata)(ExEdit::ObjectFilterIndex processing, const char* exdata_use_name);

private:
	void init_pointers(AviUtl::FilterPlugin* efp)
	{
		fp = efp;

		auto pick_addr = [exedit_base = reinterpret_cast<intptr_t>(efp->dll_hinst), this]
			<class T>(T& target, ptrdiff_t offset) { target = reinterpret_cast<T>(exedit_base + offset); };
		auto pick_call_addr = [&]<class T>(T& target, ptrdiff_t offset) {
			ptrdiff_t* tmp; pick_addr(tmp, offset);
			target = reinterpret_cast<T>(4 + reinterpret_cast<intptr_t>(tmp) + *tmp);
		};

		pick_call_addr(get_or_create_cache,	0x01c1ea);
		pick_addr(update_any_exdata,		0x04a7e0);
	}
} exedit{};


////////////////////////////////
// マルチスレッド関数のラッパー．
////////////////////////////////
constexpr auto multi_thread = [](auto&& func) {
	exedit.fp->exfunc->exec_multi_thread_func([](int thread_id, int thread_num, void* param1, void* param2) {
		auto pfunc = reinterpret_cast<decltype(&func)>(param1);
		(*pfunc)(thread_id, thread_num);
	}, &func, nullptr);
};


////////////////////////////////
// 仕様書．
////////////////////////////////
struct check_data {
	enum def : int32_t {
		unchecked = 0,
		checked = 1,
		button = -1,
		dropdown = -2,
	};
};

enum class ConvKernel : uint8_t {
	rectangular,
	triangular,
	Hann,
	Hamming,
	Blackman,
	Gaussian,
};
constexpr int count_kernels = 6;

#define PLUGIN_VERSION	"v1.04"
#define PLUGIN_AUTHOR	"sigma-axis"
#define FILTER_INFO_FMT(name, ver, author)	(name##" "##ver##" by "##author)
#define FILTER_INFO(name)	constexpr char filter_name[] = name, info[] = FILTER_INFO_FMT(name, PLUGIN_VERSION, PLUGIN_AUTHOR)
FILTER_INFO("簡易ローパスハイパス");

// trackbars.
constexpr const char* track_names[] = { "周波数", "強さ", "補正dB", "最小遅延" };
constexpr int32_t
	track_denom[]    = {   100,   10,   100,  1 },
	track_min[]      = { -4800,    0, -1000,  0 },
	track_min_drag[] = { -2400,    0,     0,  0 },
	track_default[]  = { +1200, 1000,     0, 40 },
	track_max_drag[] = { +4800, 1000, +2000, 40 },
	track_max[]      = { +6000, 1000, +4000, 40 };

static_assert(
	std::size(track_names) == std::size(track_denom) &&
	std::size(track_names) == std::size(track_min) &&
	std::size(track_names) == std::size(track_min_drag) &&
	std::size(track_names) == std::size(track_default) &&
	std::size(track_names) == std::size(track_max_drag) &&
	std::size(track_names) == std::size(track_max));

namespace idx_track
{
	enum id : int {
		frequency,
		intensity,
		volume,
		latency,
	};
};

// checks.
constexpr auto name_conv_kernel = "窓関数";
constexpr const char* check_names[]
	= { "ハイパス", "矩形窓\0三角窓\0Hann窓\0Hamming窓\0Blackman窓\0Gauss窓\0" };
constexpr int32_t check_default[] = { check_data::unchecked, check_data::dropdown };
static_assert(std::size(check_names) == std::size(check_default));

namespace idx_check
{
	enum id : int {
		hipass,
		kernel,
	};
};

// exdata.
constexpr ExEdit::ExdataUse exdata_use[] =
{
	{ .type = ExEdit::ExdataUse::Type::Number, .size = 1, .name = "kernel" },
};

//#pragma pack(push, 1)
struct Exdata {
	ConvKernel kernel = ConvKernel::Hann;
};

//#pragma pack(pop)
constexpr static Exdata exdata_def = {};

static_assert(sizeof(Exdata) == std::accumulate(
	std::begin(exdata_use), std::end(exdata_use), size_t{ 0 }, [](auto v, auto d) { return v + d.size; }));

BOOL func_proc(ExEdit::Filter* efp, ExEdit::FilterProcInfo* efpip);
BOOL func_WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, AviUtl::EditHandle* editp, ExEdit::Filter* efp);
int32_t func_window_init(HINSTANCE hinstance, HWND hwnd, int y, int base_id, int sw_param, ExEdit::Filter* efp);

consteval ExEdit::Filter filter_info(ExEdit::Filter::Flag flag, bool do_init = true) {
	return {
		.flag = flag,
		.name = const_cast<char*>(filter_name),
		.track_n = std::size(track_names),
		.track_name = const_cast<char**>(track_names),
		.track_default = const_cast<int*>(track_default),
		.track_s = const_cast<int*>(track_min),
		.track_e = const_cast<int*>(track_max),
		.check_n = std::size(check_names),
		.check_name = const_cast<char**>(check_names),
		.check_default = const_cast<int*>(check_default),
		.func_proc = &func_proc,
		.func_init = do_init ? [](ExEdit::Filter* efp) { exedit.init(efp->exedit_fp); return TRUE; } : nullptr,
		.func_WndProc = &func_WndProc,
		.exdata_size = sizeof(exdata_def),
		.information = const_cast<char*>(info),
		.func_window_init = &func_window_init,
		.exdata_def = const_cast<Exdata*>(&exdata_def),
		.exdata_use = exdata_use,
		.track_scale = const_cast<int*>(track_denom),
		.track_drag_min = const_cast<int*>(track_min_drag),
		.track_drag_max = const_cast<int*>(track_max_drag),
	};
}
inline constinit auto
	filter = filter_info(ExEdit::Filter::Flag::Audio, false),
	effect = filter_info(ExEdit::Filter::Flag::Audio | ExEdit::Filter::Flag::Effect);


////////////////////////////////
// ウィンドウ状態の保守．
////////////////////////////////
static void update_window_state(ExEdit::Filter* efp)
{
	/*
	efp->exfunc->get_hwnd(efp->processing, i, j):
		i = 0:		j 番目のスライダーの中央ボタン．
		i = 1:		j 番目のスライダーの左トラックバー．
		i = 2:		j 番目のスライダーの右トラックバー．
		i = 3:		j 番目のチェック枠のチェックボックス．
		i = 4:		j 番目のチェック枠のボタン．
		i = 5, 7:	j 番目のチェック枠の右にある static (テキスト).
		i = 6:		j 番目のチェック枠のコンボボックス．
		otherwise -> nullptr.
	*/

	auto* exdata = reinterpret_cast<Exdata*>(efp->exdata_ptr);

	// 「窓関数」のコンボボックスの項目選択.
	if (int idx = static_cast<int>(exdata->kernel); 0 <= idx && idx < count_kernels) {
		::SendMessageA(efp->exfunc->get_hwnd(efp->processing, 6, idx_check::kernel), CB_SETCURSEL, idx, 0);
	}

	// コンボボックス横のテキスト設定.
	::SetWindowTextA(efp->exfunc->get_hwnd(efp->processing, 7, idx_check::kernel), name_conv_kernel);
}

BOOL func_WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, AviUtl::EditHandle* editp, ExEdit::Filter* efp)
{
	if (message != ExEdit::ExtendedFilter::Message::WM_EXTENDEDFILTER_COMMAND) return FALSE;

	auto* exdata = reinterpret_cast<Exdata*>(efp->exdata_ptr);
	auto chk = static_cast<idx_check::id>(wparam >> 16);
	auto cmd = wparam & 0xffff;

	switch (cmd) {
		using namespace ExEdit::ExtendedFilter::CommandId;
	case EXTENDEDFILTER_SELECT_DROPDOWN:
		if (chk == idx_check::kernel) {
			// コンボボックスからの選択項目を取得，「元に戻す」にデータ記録．
			auto ker = static_cast<ConvKernel>(std::clamp(static_cast<int>(lparam), 0, count_kernels - 1));
			if (exdata->kernel != ker) {
				efp->exfunc->set_undo(efp->processing, 0);
				exdata->kernel = ker;
				exedit.update_any_exdata(efp->processing, exdata_use[0].name);

				update_window_state(efp);
			}
			return TRUE;
		}
		break;
	}
	return TRUE;
}

int32_t func_window_init(HINSTANCE hinstance, HWND hwnd, int y, int base_id, int sw_param, ExEdit::Filter* efp)
{
	if (sw_param != 0) update_window_state(efp);
	return 0;
}


////////////////////////////////
// フィルタ処理．
////////////////////////////////
BOOL func_proc(ExEdit::Filter* efp, ExEdit::FilterProcInfo* efpip)
{
	int constexpr
		den_freq		= track_denom[idx_track::frequency],
		max_intensity	= track_max[idx_track::intensity],
		den_volume		= track_denom[idx_track::volume],
		den_latency		= track_denom[idx_track::latency];

	int const
		raw_freq		= efp->track[idx_track::frequency],
		raw_intensity	= efp->track[idx_track::intensity],
		raw_volume		= efp->track[idx_track::volume],
		raw_latency		= efp->track[idx_track::latency];
	bool const hipass = efp->check[idx_check::hipass] != check_data::unchecked;
	auto const kernel = reinterpret_cast<Exdata*>(efp->exdata_ptr)->kernel;

	float const intensity = raw_intensity / (1.0f * max_intensity),
		volume = raw_volume / (20.0f * den_volume);

	int constexpr
		max_window_len = 2 * (track_max[idx_track::latency] / den_latency); // 畳み込み積の幅の最大想定 (ミリ秒).

	// 過去音声データを保存するキャッシュを取得．ほとんどエコーフィルタ効果のソースのコピー．
	// https://github.com/nazonoSAUNA/Echo.eef/blob/main/src.cpp
	using i16 = int16_t;
	struct {
		int offset;
		int frame;
		i16 data[1];
	}* cache;
	int const max_window_size = efpip->audio_rate * max_window_len / 1000 + 1,
		buffer_size = std::bit_ceil(std::clamp<uint32_t>(
			2 * max_window_size * efpip->audio_ch, 0x1000, 0x800'0000));
	int cache_exists_flag;
	cache = reinterpret_cast<decltype(cache)>(exedit.get_or_create_cache(
		efp->processing, buffer_size + (offsetof(std::decay_t<decltype(*cache)>, data) / sizeof(i16)), 1, 8 * sizeof(i16),
		efpip->v_func_idx, &cache_exists_flag));

	if (cache == nullptr) return TRUE; // キャッシュを取得できなかった場合何もしない．

	if (int const frame = efpip->frame + efpip->add_frame;
		frame == 0 || frame < cache->frame || cache_exists_flag == 0) {
		// 新規キャッシュ，あるいは時間が巻き戻っているなら初期化．
		cache->offset = 0;
		cache->frame = frame;
		std::memset(cache->data, 0, buffer_size * sizeof(i16));
	}
	else if (cache->frame == frame) {
		// 同一フレームなので音声ソースの更新．書き込み場所を巻き戻す．
		cache->offset -= std::min(max_window_size, efpip->audio_n) * efpip->audio_ch;
		cache->offset &= (buffer_size - 1);
	}
	else cache->frame = frame;

	// キャッシュへ元音声の末尾データを追記するラムダ．
	auto push_cache = [&](i16 const* src) {
		int L = std::min(max_window_size, efpip->audio_n) * efpip->audio_ch,
			i0 = efpip->audio_n * efpip->audio_ch - L;
		for (int i = 0; i < L; i++) {
			cache->data[cache->offset] = src[i + i0];
			cache->offset = (cache->offset + 1) & (buffer_size - 1);
		}
	};

	// 強さ 0 の場合はキャッシュの更新だけして音声加工の必要なし．
	if (intensity <= 0) {
		push_cache(efp == &effect ? efpip->audio_data : efpip->audio_p);
		return TRUE;
	}

	// 計算に必要なサンプル数をおおまかに計算．
	float const freq = 440.0f * std::exp2f(raw_freq / (12.0f * den_freq)), // 440Hz A を基準．
		num_samples_rough = efpip->audio_rate / freq;

	// 選択肢に応じて畳み込み関数のテーブル作成．
	std::vector<float> ker_func{};
	auto set_size = [&](float scale) {
		ker_func.resize(std::clamp(static_cast<int>(num_samples_rough * scale), 1, buffer_size / efpip->audio_ch));
	};
	auto constexpr pi = std::numbers::pi_v<float>;
	switch (kernel) {
	case ConvKernel::rectangular:
	{
		// 矩形窓．
		set_size(1.0f);
		auto const A = 1 / (1.0f * ker_func.size());
		for (size_t i = 0; i < ker_func.size(); i++) {
			ker_func[i] = A;
		}
		break;
	}
	case ConvKernel::triangular:
	{
		// 三角窓．
		set_size(1.0f);
		auto const A = 1 / (0.50f * ker_func.size());
		for (size_t i = 0; i < ker_func.size(); i++) {
			auto v = 2 * (i + 0.5f) / ker_func.size();
			ker_func[i] = A * (v < 1 ? v : 2 - v);
		}
		break;
	}
	case ConvKernel::Hann:
	default:
	{
		// Hann.
		set_size(1.0f);
		auto const A = 1 / (0.50f * ker_func.size()),
			cf = 2 * pi / ker_func.size();
		for (size_t i = 0; i < ker_func.size(); i++) {
			ker_func[i] = A * (0.50f - 0.50f * std::cosf(cf * (i + 0.5f)));
		}
		break;
	}
	case ConvKernel::Hamming:
	{
		// Hamming.
		set_size(1.0f);
		auto const A = 1 / (0.54f * ker_func.size()),
			cf = 2 * pi / ker_func.size();
		for (size_t i = 0; i < ker_func.size(); i++) {
			ker_func[i] = A * (0.54f - 0.46f * std::cosf(cf * (i + 0.5f)));
		}
		break;
	}
	case ConvKernel::Blackman:
	{
		// Blackman 窓．
		set_size(1.0f);
		auto const A = 1 / (0.42f * ker_func.size()),
			cf = 2 * pi / ker_func.size();
		for (size_t i = 0; i < ker_func.size(); i++) {
			ker_func[i] = A * (0.42f
				- 0.50f * std::cosf(cf * (i + 0.5f))
				+ 0.08f * std::cosf(2 * cf * (i + 0.5f)));
		}
		break;
	}
	case ConvKernel::Gaussian:
	{
		// ガウスぼかし．
		set_size(2.0f);
		auto const A = 4.0f / ker_func.size();
		for (size_t i = 0; i < ker_func.size(); i++) {
			float x = 4.0f * ((i + 0.5f) / ker_func.size() - 0.5f);
			ker_func[i] = A * std::expf(-pi * x * x);
		}
		break;
	}
	}

	// ハイパスフィルタへ転換（Dirac δ との補数をとるだけ）．
	if (hipass) {
		for (auto& v : ker_func) v *= -1;
		ker_func[(ker_func.size() - 1) >> 1] += 1;
	}

	// "強さ" と "補正dB" の加味．
	if (intensity < 1 || volume != 0) {
		auto const rate = intensity * std::powf(10, volume);
		for (auto& v : ker_func) v *= rate;
	}

	// 元音声データを一旦退避，書き込み先を取得．ここ以降，元音声の参照先は efpip->audio_temp.
	i16* data;
	if (efp == &effect) {
		std::swap(efpip->audio_data, efpip->audio_temp);
		data = efpip->audio_data;
	}
	else {
		std::memcpy(efpip->audio_temp, efpip->audio_p,
			sizeof(i16) * efpip->audio_n * efpip->audio_ch);
		data = efpip->audio_p;
	}

	// 畳み込みの計算．
	int const num_samples = efpip->audio_n, num_ch = efpip->audio_ch,
		// 既知の問題:
		// 「周波数」を時間的に変化させるとプツプツノイズが入る．
		// ピークに対応するサンプル位置が func_proc() ごとに不連続に飛んでしまうのが理由の模様．
		// ->「最小遅延」のパラメタ調整で抑えることにした．
		latency = std::max<int>(efpip->audio_rate * raw_latency / (1000 * den_latency) - ((ker_func.size() - 1) >> 1), 0) * num_ch;
	multi_thread([&](int thread_id, int thread_num) {
		for (int i = thread_id; i < num_samples; i += thread_num) {
			int const dst_offset = i * num_ch, // 書き込み先．
				src_offset = dst_offset - latency; // 対応する読み込み元．

			for (int c = 0; c < num_ch; c++) {

				// 畳み込み積分 ("強さ" が 100% でない場合の余剰分が和の初期値).
				float sum = (1 - intensity) * efpip->audio_temp[dst_offset + c];
				for (int j = ker_func.size(); --j >= 0;) {
					int J = src_offset + c - j * num_ch;
					auto src = J < 0 ? cache->data[(cache->offset + J) & (buffer_size - 1)] : efpip->audio_temp[J];
					sum = std::fmaf(src, ker_func[j], sum);
				}

				// 出力データに書き出し．
				data[dst_offset + c] = static_cast<i16>(std::clamp<float>(sum,
					std::numeric_limits<i16>::min(), std::numeric_limits<i16>::max()));
			}
		}
	});

	// キャッシュを更新．
	push_cache(efpip->audio_temp);

	return TRUE;
}


////////////////////////////////
// 初期化．
////////////////////////////////
BOOL WINAPI DllMain(HINSTANCE hinst, DWORD fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason) {
	case DLL_PROCESS_ATTACH:
		::DisableThreadLibraryCalls(hinst);
		break;
	}
	return TRUE;
}


////////////////////////////////
// エントリポイント．
////////////////////////////////
EXTERN_C __declspec(dllexport) ExEdit::Filter* const* __stdcall GetFilterTableList() {
	constexpr static ExEdit::Filter* filter_list[] = {
		&filter,
		&effect,
		nullptr,
	};

	return filter_list;
}

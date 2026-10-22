/*
 * CLCL Installer
 *
 * unzip.c
 *
 * Copyright (C) 1996-2026 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

/* Include Files */
#include <windows.h>
#include <tchar.h>

#include "unzip.h"

/* Define */
#define MAXBITS						15			// ハフマン符号の最大ビット長
#define MAXLCODES					286			// リテラル/長さ符号の最大数
#define MAXDCODES					30			// 距離符号の最大数
#define MAXCODES					(MAXLCODES + MAXDCODES)
#define FIXLCODES					288			// 固定ハフマンのリテラル/長さ符号数

#define SIG_LOCAL					0x04034b50
#define SIG_CENTRAL					0x02014b50
#define SIG_EOCD					0x06054b50

/* Struct */
typedef struct _HUFFMAN {
	short *count;								// ビット長ごとの符号数
	short *symbol;								// canonical順のシンボル
} HUFFMAN;

typedef struct _INFLATE_STATE {
	const BYTE *in;								// 入力
	DWORD in_len;
	DWORD in_cnt;
	int bit_buf;								// ビットバッファ
	int bit_cnt;
	BYTE *out;									// 出力
	DWORD out_len;
	DWORD out_cnt;
} INFLATE_STATE;

/* Global Variables */

/* Local Function Prototypes */
static DWORD get_dword(const BYTE *p);
static WORD get_word(const BYTE *p);
static DWORD calc_crc32(const BYTE *buf, const DWORD size);
static int inflate_bits(INFLATE_STATE *s, const int need);
static int inflate_stored(INFLATE_STATE *s);
static int huffman_construct(HUFFMAN *h, const short *length, const int n);
static int huffman_decode(INFLATE_STATE *s, const HUFFMAN *h);
static int inflate_codes(INFLATE_STATE *s, const HUFFMAN *lencode, const HUFFMAN *distcode);
static int inflate_fixed(INFLATE_STATE *s);
static int inflate_dynamic(INFLATE_STATE *s);
static int inflate(BYTE *out, const DWORD out_len, const BYTE *in, const DWORD in_len);

/*
 * get_dword - リトルエンディアンで4バイト取得
 */
static DWORD get_dword(const BYTE *p)
{
	return (DWORD)p[0] | ((DWORD)p[1] << 8) | ((DWORD)p[2] << 16) | ((DWORD)p[3] << 24);
}

/*
 * get_word - リトルエンディアンで2バイト取得
 */
static WORD get_word(const BYTE *p)
{
	return (WORD)((WORD)p[0] | ((WORD)p[1] << 8));
}

/*
 * calc_crc32 - CRC32の計算
 */
static DWORD calc_crc32(const BYTE *buf, const DWORD size)
{
	DWORD crc = 0xffffffff;
	DWORD i;
	int j;

	for (i = 0; i < size; i++) {
		crc ^= (DWORD)buf[i];
		for (j = 0; j < 8; j++) {
			crc = (crc & 1) ? ((crc >> 1) ^ 0xedb88320) : (crc >> 1);
		}
	}
	return crc ^ 0xffffffff;
}

/*
 * inflate_bits - 指定ビット数を取り出す
 */
static int inflate_bits(INFLATE_STATE *s, const int need)
{
	long val = s->bit_buf;

	while (s->bit_cnt < need) {
		if (s->in_cnt == s->in_len) {
			return -1;
		}
		val |= (long)(s->in[s->in_cnt++]) << s->bit_cnt;
		s->bit_cnt += 8;
	}
	s->bit_buf = (int)(val >> need);
	s->bit_cnt -= need;
	return (int)(val & ((1L << need) - 1));
}

/*
 * inflate_stored - 非圧縮ブロックの展開
 */
static int inflate_stored(INFLATE_STATE *s)
{
	unsigned len;

	s->bit_buf = 0;
	s->bit_cnt = 0;

	if (s->in_cnt + 4 > s->in_len) {
		return -1;
	}
	len = get_word(s->in + s->in_cnt);
	if (s->in[s->in_cnt + 2] != (~len & 0xff) || s->in[s->in_cnt + 3] != ((~len >> 8) & 0xff)) {
		return -1;
	}
	s->in_cnt += 4;

	if (s->in_cnt + len > s->in_len || s->out_cnt + len > s->out_len) {
		return -1;
	}
	CopyMemory(s->out + s->out_cnt, s->in + s->in_cnt, len);
	s->in_cnt += len;
	s->out_cnt += len;
	return 0;
}

/*
 * huffman_construct - ビット長の並びからハフマンテーブルを作成
 */
static int huffman_construct(HUFFMAN *h, const short *length, const int n)
{
	int symbol;
	int len;
	int left;
	short offs[MAXBITS + 1];

	for (len = 0; len <= MAXBITS; len++) {
		h->count[len] = 0;
	}
	for (symbol = 0; symbol < n; symbol++) {
		h->count[length[symbol]]++;
	}
	if (h->count[0] == n) {
		// 符号が無い
		return 0;
	}

	// 符号が完全かどうかの確認
	left = 1;
	for (len = 1; len <= MAXBITS; len++) {
		left <<= 1;
		left -= h->count[len];
		if (left < 0) {
			// 符号が多すぎる
			return left;
		}
	}

	offs[1] = 0;
	for (len = 1; len < MAXBITS; len++) {
		offs[len + 1] = offs[len] + h->count[len];
	}
	for (symbol = 0; symbol < n; symbol++) {
		if (length[symbol] != 0) {
			h->symbol[offs[length[symbol]]++] = (short)symbol;
		}
	}
	return left;
}

/*
 * huffman_decode - ハフマン符号を1シンボル読み込む
 */
static int huffman_decode(INFLATE_STATE *s, const HUFFMAN *h)
{
	int len;
	int code = 0;
	int first = 0;
	int count;
	int index = 0;
	int bit;

	for (len = 1; len <= MAXBITS; len++) {
		if ((bit = inflate_bits(s, 1)) < 0) {
			return -1;
		}
		code |= bit;
		count = h->count[len];
		if (code - count < first) {
			return h->symbol[index + (code - first)];
		}
		index += count;
		first += count;
		first <<= 1;
		code <<= 1;
	}
	return -1;
}

/*
 * inflate_codes - ハフマン符号のブロックを展開
 */
static int inflate_codes(INFLATE_STATE *s, const HUFFMAN *lencode, const HUFFMAN *distcode)
{
	static const short lens[29] = {
		3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
		35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258 };
	static const short lext[29] = {
		0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
		3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0 };
	static const short dists[30] = {
		1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
		257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577 };
	static const short dext[30] = {
		0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
		7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13 };
	int symbol;
	int len;
	int dist;
	int bits;

	for (;;) {
		if ((symbol = huffman_decode(s, lencode)) < 0) {
			return -1;
		}
		if (symbol < 256) {
			// リテラル
			if (s->out_cnt == s->out_len) {
				return -1;
			}
			s->out[s->out_cnt++] = (BYTE)symbol;
			continue;
		}
		if (symbol == 256) {
			// ブロックの終端
			break;
		}
		// 長さ
		symbol -= 257;
		if (symbol >= 29) {
			return -1;
		}
		if ((bits = inflate_bits(s, lext[symbol])) < 0) {
			return -1;
		}
		len = lens[symbol] + bits;

		// 距離
		if ((symbol = huffman_decode(s, distcode)) < 0 || symbol >= 30) {
			return -1;
		}
		if ((bits = inflate_bits(s, dext[symbol])) < 0) {
			return -1;
		}
		dist = dists[symbol] + bits;
		if ((DWORD)dist > s->out_cnt || s->out_cnt + len > s->out_len) {
			return -1;
		}
		for (; len > 0; len--) {
			s->out[s->out_cnt] = s->out[s->out_cnt - dist];
			s->out_cnt++;
		}
	}
	return 0;
}

/*
 * inflate_fixed - 固定ハフマンのブロックを展開
 */
static int inflate_fixed(INFLATE_STATE *s)
{
	short lencnt[MAXBITS + 1], lensym[FIXLCODES];
	short distcnt[MAXBITS + 1], distsym[MAXDCODES];
	short lengths[FIXLCODES];
	HUFFMAN lencode, distcode;
	int symbol;

	lencode.count = lencnt;
	lencode.symbol = lensym;
	distcode.count = distcnt;
	distcode.symbol = distsym;

	for (symbol = 0; symbol < 144; symbol++) {
		lengths[symbol] = 8;
	}
	for (; symbol < 256; symbol++) {
		lengths[symbol] = 9;
	}
	for (; symbol < 280; symbol++) {
		lengths[symbol] = 7;
	}
	for (; symbol < FIXLCODES; symbol++) {
		lengths[symbol] = 8;
	}
	huffman_construct(&lencode, lengths, FIXLCODES);

	for (symbol = 0; symbol < MAXDCODES; symbol++) {
		lengths[symbol] = 5;
	}
	huffman_construct(&distcode, lengths, MAXDCODES);

	return inflate_codes(s, &lencode, &distcode);
}

/*
 * inflate_dynamic - 動的ハフマンのブロックを展開
 */
static int inflate_dynamic(INFLATE_STATE *s)
{
	static const short order[19] = { 16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15 };
	short lencnt[MAXBITS + 1], lensym[MAXLCODES];
	short distcnt[MAXBITS + 1], distsym[MAXDCODES];
	short lengths[MAXCODES];
	HUFFMAN lencode, distcode;
	int nlen, ndist, ncode;
	int index;
	int symbol;
	int len;
	int err;

	lencode.count = lencnt;
	lencode.symbol = lensym;
	distcode.count = distcnt;
	distcode.symbol = distsym;

	if ((index = inflate_bits(s, 5)) < 0) {
		return -1;
	}
	nlen = index + 257;
	if ((index = inflate_bits(s, 5)) < 0) {
		return -1;
	}
	ndist = index + 1;
	if ((index = inflate_bits(s, 4)) < 0) {
		return -1;
	}
	ncode = index + 4;
	if (nlen > MAXLCODES || ndist > MAXDCODES) {
		return -1;
	}

	// 符号長のハフマンテーブル
	for (index = 0; index < ncode; index++) {
		if ((len = inflate_bits(s, 3)) < 0) {
			return -1;
		}
		lengths[order[index]] = (short)len;
	}
	for (; index < 19; index++) {
		lengths[order[index]] = 0;
	}
	if (huffman_construct(&lencode, lengths, 19) != 0) {
		return -1;
	}

	// リテラル/長さと距離の符号長を読み込む
	index = 0;
	while (index < nlen + ndist) {
		if ((symbol = huffman_decode(s, &lencode)) < 0) {
			return -1;
		}
		if (symbol < 16) {
			lengths[index++] = (short)symbol;
			continue;
		}
		len = 0;
		if (symbol == 16) {
			if (index == 0) {
				return -1;
			}
			len = lengths[index - 1];
			if ((symbol = inflate_bits(s, 2)) < 0) {
				return -1;
			}
			symbol += 3;
		} else if (symbol == 17) {
			if ((symbol = inflate_bits(s, 3)) < 0) {
				return -1;
			}
			symbol += 3;
		} else {
			if ((symbol = inflate_bits(s, 7)) < 0) {
				return -1;
			}
			symbol += 11;
		}
		if (index + symbol > nlen + ndist) {
			return -1;
		}
		for (; symbol > 0; symbol--) {
			lengths[index++] = (short)len;
		}
	}
	if (lengths[256] == 0) {
		return -1;
	}

	if ((err = huffman_construct(&lencode, lengths, nlen)) != 0 && (err < 0 || nlen != lencnt[0] + lencnt[1])) {
		return -1;
	}
	if ((err = huffman_construct(&distcode, lengths + nlen, ndist)) != 0 && (err < 0 || ndist != distcnt[0] + distcnt[1])) {
		return -1;
	}
	return inflate_codes(s, &lencode, &distcode);
}

/*
 * inflate - deflateデータの展開
 */
static int inflate(BYTE *out, const DWORD out_len, const BYTE *in, const DWORD in_len)
{
	INFLATE_STATE s;
	int last, type;
	int ret = 0;

	s.in = in;
	s.in_len = in_len;
	s.in_cnt = 0;
	s.bit_buf = 0;
	s.bit_cnt = 0;
	s.out = out;
	s.out_len = out_len;
	s.out_cnt = 0;

	do {
		if ((last = inflate_bits(&s, 1)) < 0 || (type = inflate_bits(&s, 2)) < 0) {
			return -1;
		}
		switch (type) {
		case 0:
			ret = inflate_stored(&s);
			break;
		case 1:
			ret = inflate_fixed(&s);
			break;
		case 2:
			ret = inflate_dynamic(&s);
			break;
		default:
			return -1;
		}
		if (ret != 0) {
			return ret;
		}
	} while (last == 0);

	return (s.out_cnt == out_len) ? 0 : -1;
}

/*
 * zip_open - ZIPアーカイブを開く
 */
BOOL zip_open(const BYTE *data, const DWORD size, ZIP_ARCHIVE *za)
{
	const BYTE *p;
	DWORD cd_offset;
	DWORD cd_size;
	DWORD i;

	if (data == NULL || size < 22) {
		return FALSE;
	}
	// End of central directory の検索
	for (i = 0; i <= size - 22 && i <= 0xffff; i++) {
		p = data + (size - 22 - i);
		if (get_dword(p) == SIG_EOCD) {
			break;
		}
	}
	if (i > size - 22 || i > 0xffff) {
		return FALSE;
	}
	za->count = get_word(p + 10);
	cd_size = get_dword(p + 12);
	cd_offset = get_dword(p + 16);
	if (cd_offset > size || cd_size > size - cd_offset) {
		return FALSE;
	}
	za->data = data;
	za->size = size;
	za->central = data + cd_offset;
	return TRUE;
}

/*
 * zip_get_entry - インデックスからエントリの情報を取得
 */
BOOL zip_get_entry(const ZIP_ARCHIVE *za, const DWORD index, ZIP_ENTRY *ze)
{
	const BYTE *p;
	DWORD off = (DWORD)(za->central - za->data);
	DWORD name_len, extra_len, comment_len;
	DWORD i;
	UINT cp;
	int len;
	TCHAR *tp;

	if (index >= za->count) {
		return FALSE;
	}
	for (i = 0; i < index; i++) {
		if (off > za->size || za->size - off < 46 || get_dword(za->data + off) != SIG_CENTRAL) {
			return FALSE;
		}
		p = za->data + off;
		off += 46 + get_word(p + 28) + get_word(p + 30) + get_word(p + 32);
	}
	if (off > za->size || za->size - off < 46 || get_dword(za->data + off) != SIG_CENTRAL) {
		return FALSE;
	}
	p = za->data + off;
	name_len = get_word(p + 28);
	extra_len = get_word(p + 30);
	comment_len = get_word(p + 32);
	if (name_len + extra_len + comment_len > za->size - off - 46) {
		return FALSE;
	}

	ze->method = get_word(p + 10);
	ze->crc = get_dword(p + 16);
	ze->comp_size = get_dword(p + 20);
	ze->orig_size = get_dword(p + 24);
	ze->local_offset = get_dword(p + 42);

	// 格納パスの取得 (MAX_PATH に収まらない名前は不正とする)
	ZeroMemory(ze->name, sizeof(ze->name));
	if (name_len == 0) {
		return FALSE;
	}
	cp = (get_word(p + 8) & 0x0800) ? CP_UTF8 : CP_OEMCP;
	len = MultiByteToWideChar(cp, 0, (const char *)(p + 46), name_len, NULL, 0);
	if (len <= 0 || len >= MAX_PATH) {
		return FALSE;
	}
	MultiByteToWideChar(cp, 0, (const char *)(p + 46), name_len, ze->name, MAX_PATH - 1);
	if (ze->name[0] == TEXT('\0')) {
		return FALSE;
	}
	for (tp = ze->name; *tp != TEXT('\0'); tp++) {
		if (*tp == TEXT('/')) {
			*tp = TEXT('\\');
		}
	}
	ze->is_dir = (*(tp - 1) == TEXT('\\')) ? TRUE : FALSE;
	if (ze->is_dir == TRUE) {
		*(tp - 1) = TEXT('\0');
	}
	return TRUE;
}

/*
 * zip_extract - エントリをメモリ上に展開
 */
int zip_extract(const ZIP_ARCHIVE *za, const ZIP_ENTRY *ze, BYTE **buf, DWORD *buf_size)
{
	const BYTE *p;
	const BYTE *data;
	BYTE *out;
	DWORD off;

	*buf = NULL;
	*buf_size = 0;
	if (ze->is_dir == TRUE) {
		return UNZIP_OK;
	}
	if (ze->method != 0 && ze->method != 8) {
		return UNZIP_ERR_METHOD;
	}
	// ローカルヘッダから実データの位置を取得 (加算で桁あふれしないように比較する)
	if (za->size < 30 || ze->local_offset > za->size - 30) {
		return UNZIP_ERR_FORMAT;
	}
	p = za->data + ze->local_offset;
	if (get_dword(p) != SIG_LOCAL) {
		return UNZIP_ERR_FORMAT;
	}
	off = (DWORD)get_word(p + 26) + get_word(p + 28);
	if (off > za->size - 30 - ze->local_offset) {
		return UNZIP_ERR_FORMAT;
	}
	off += ze->local_offset + 30;
	if (ze->comp_size > za->size - off) {
		return UNZIP_ERR_FORMAT;
	}
	data = za->data + off;

	if ((out = (BYTE *)HeapAlloc(GetProcessHeap(), 0, (ze->orig_size == 0) ? 1 : ze->orig_size)) == NULL) {
		return UNZIP_ERR_MEMORY;
	}
	if (ze->method == 0) {
		if (ze->comp_size != ze->orig_size) {
			HeapFree(GetProcessHeap(), 0, out);
			return UNZIP_ERR_FORMAT;
		}
		CopyMemory(out, data, ze->orig_size);
	} else {
		if (inflate(out, ze->orig_size, data, ze->comp_size) != 0) {
			HeapFree(GetProcessHeap(), 0, out);
			return UNZIP_ERR_DATA;
		}
	}
	if (calc_crc32(out, ze->orig_size) != ze->crc) {
		HeapFree(GetProcessHeap(), 0, out);
		return UNZIP_ERR_CRC;
	}
	*buf = out;
	*buf_size = ze->orig_size;
	return UNZIP_OK;
}

/* End of source */

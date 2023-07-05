永続メモリの不適切な永続化処理を検知するデバッグツール

デバッグツールの構成要素となるソースファイル:
wraplibpmem.c
wraplibpmem.h
wraplibpmemobj.c
wraplibpmemobj.h
PmemWrap_memcpy.c
sema_state_generate.py
st_replace.py
no_pmdk_api.h

デバッグツールを使用するために元のコードから改変したヘッダファイル:
libpmem.h
libpmemobj.j

wraplibpmem.c と wraplibpmemobj.c から libwrappmem.so がコンパイルされる。
libwrapmem.so を検証対象のプログラムにリンクし、コンパイル時に -fsanitize=address -g -ggdb を追加する。その後、シェルスクリプトを使ってコンパイルされた実行ファイルを実行する。その際、1度目の実行で自動でクラッシュさせ、2度目の実行で異常終了するか確認する。2度目に異常終了すれば不具合として記録する。この流れを繰り返し、不具合があるか検証する。
PmemWrap_memcpy.c は1度目の実行の後、永続化処理前の更新を操作し、クラッシュ時に可能性としてあり得るデータを生成し、不具合を誘発させるために使用する。

libpmem.h と libpmemobj.h は 永続メモリライブラリ群 PMDK で標準的に使用されるヘッダファイルだが、本ツールを使用するために改変し、コンパイル時にこれらのヘッダファイルをインクルードするようにしている。
﻿#pragma once

#include <memory>

#include "IDeletable.h"
#include "ITexture.h"
#include "IRenderTexture.h"
#include "IMaterial.h"
#include "IShader.h"
#include "IShape.h"
#include "IGraphicPipeline.h"
#include "GraphicPipelineDesc.h"

namespace og
{


	/// <summary>
	/// DirectXやOpenGLのようなグラフィックAPIを共通の関数を通して扱えるようにするための抽象クラス
	/// </summary>
	class IGraphicWrapper :public IDeletable
	{
	public:
		/// <summary>
		/// グラフィックライブラリの初期化。
		/// </summary>
		/// <remarks>
		/// ライブラsリを使用するには、まずこの関数を呼び出す必要があります。この関数を使ってエラーが返ってきた場合は直ちにアプリを終了すること。
		/// </remarks>
		/// <returns>　０：成功<br/>－１：エラー</returns>
		virtual S32 Init() = 0;

		/// <summary>
		/// フリップ関数。裏画面を表画面に反映する。
		/// </summary>
		/// <returns>　０：成功\n－１：エラー</returns>
		virtual S32 SwapScreen(SPtr<IRenderTexture>& renderTarget) = 0;


		//===================================================================================//

		/// <summary>
		/// 書き込み可能なテクスチャを生成。
		/// </summary>
		/// <remarks>
		/// 関数が成功するとレンダーテクスチャオブジェクトが返される。生成に失敗した場合は無効なオブジェクトが返される。
		/// </remarks>
		/// <param name="width">横幅</param>
		/// <param name="height">高さ</param>
		/// <param name="format">生成する画像のピクセルフォーマット</param>
		/// <returns>レンダーテクスチャオブジェクト</returns>
		virtual SPtr<IRenderTexture> CreateRenderTexture(const S32 width, const S32 height, const TextureFormat format) = 0;
		/// <summary>
		/// 書き込み可能なテクスチャを生成。
		/// </summary>
		/// <remarks>
		/// 関数が成功するとレンダーテクスチャオブジェクトが返される。生成に失敗した場合は無効なオブジェクトが返される。
		/// </remarks>
		/// <param name="width">横幅</param>
		/// <param name="height">高さ</param>
		/// <param name="formats">マルチレンダーターゲットで使用する、各レンダーターゲットのピクセルフォーマット</param>
		/// <returns>レンダーテクスチャオブジェクト</returns>
		virtual SPtr<IRenderTexture> CreateRenderTexture(const S32 width, const S32 height, const ArrayList<TextureFormat>& formats) = 0;
		/// <summary>
		/// 画像ファイルの読み込み。
		/// </summary>
		/// <remarks>
		/// グラフィックデータをGPU上に確保する。関数が成功するとライブラリから割り当てられたIDを返す。
		/// </remarks>
		/// <param name="path">ファイルパス</param>
		/// <returns>　－１　　　:　エラー\n－１以外　:　ID</returns>
		virtual SPtr<ITexture> LoadTexture(const Path& path, const bool async = false) = 0;

		//===================================================================================//

		/// <summary>
		/// シェーダの読み込み。
		/// </summary>
		/// <remarks>読み込まれたシェーダがコンパイル済みであればそのまま読み込み、そうでなければHLSL5.0でコンパイルする。関数が成功するとライブラリから割り当てられたIDを返す。コンパイルに失敗した場合はエラー内容を文字列としてerrorDestに出力する。</remarks>
		/// <param name="path">ファイルパス</param>
		/// <param name="type">シェーダの種類</param>
		/// <param name="errorDest">エラーログ</param>
		/// <returns>　－１　　　:　エラー\n－１以外　:　ID</returns>
		virtual SPtr<IShader> LoadShader(const String& path, ShaderType type, String& errorDest) = 0;

		/// <summary>
		/// 文字列からシェーダを作成。
		/// </summary>
		/// <remarks>
		/// 入力された文字列をHLSL5.0でコンパイルする。関数が成功するとライブラリから割り当てられたIDを返す。コンパイルに失敗した場合はエラー内容を文字列としてerrorDestに出力する。
		/// </remarks>
		/// <param name="src">ファイルパス</param>
		/// <param name="type">シェーダの種類</param>
		/// <param name="errorDest">エラーログ</param>
		/// <returns>　－１　　　:　エラー\n－１以外　:　ID</returns>
		virtual SPtr<IShader> CreateShader(const String& src, ShaderType type, String& errorDest) = 0;

		//===================================================================================//

		/// <summary>
		/// グラフィックパイプラインの作成。
		/// </summary>
		/// <remarks>
		/// 各シェーダを組み合わせ、グラフィックパイプラインを構築する。関数が成功するとライブラリから割り当てられたIDを返す。シェーダIDに-1を指定すると、標準のシェーダが適用される。
		/// </remarks>
		/// <param name="desc">グラフィックパイプラインの定義データ</param>
		/// <returns>　－１　　　:　エラー\n－１以外　:　ID</returns>
		virtual SPtr<IGraphicPipeline> CreateGraphicPipeline(const GraphicPipelineDesc& desc) = 0;

		//===================================================================================//

		/// <summary>
		/// グラフィックパイプラインからマテリアルを作成する。
		/// </summary>
		/// <remarks>
		/// ビットマスクでマテリアルに含めるレジスタを指定することで、アフィン行列、テクスチャ、色など個別に設定できるマテリアルを作成できる。
		/// </remarks>
		/// <param name="pipeline">作成元のグラフィックパイプライン</param>
		/// <param name="cBufferMask">定数バッファのどのレジスタをマテリアルに含めるかをビットマスクで指定する</param>
		/// <param name="texMask">テクスチャのどのレジスタをマテリアルに含めるかをビットマスクで指定する</param>
		/// <returns>新しいマテリアル</returns>
		virtual SPtr<IMaterial> CreateMaterial(const SPtr<IGraphicPipeline>& pipeline, const S32 cBufferMask = -1, const S32 texMask = -1) = 0;

		//===================================================================================//

		/// <summary>
		/// シェイプオブジェクトの生成。
		/// </summary>
		/// <remarks>
		/// 生成に失敗した場合は無効なオブジェクトが返される。
		/// </remarks>
		/// <param name="stribeSize">ストライブサイズ。頂点情報(Vector4)と法線情報(Vector3)を使用するのであればsizeof(Vector4)+sizeof(Vector3)がストライブサイズとなる。データアラインメントに注意すること。</param>
		/// <returns></returns>
		virtual SPtr<IShape> CreateShape(const U32 stribeSize) = 0;


		//===================================================================================//



		//===================================================================================//

		virtual ~IGraphicWrapper() {};
	};
}
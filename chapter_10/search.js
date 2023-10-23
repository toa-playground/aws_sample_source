/*
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: MIT-0
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

// echonet-lite モジュールを EL という名前でインポート
const EL = require('echonet-lite');

// 自分自身のオブジェクト種別指定
const myObject = '05ff01'; // オブジェクト種別 '05ff01' は「コントローラーオブジェクト」

// 自分のオブジェクトを指定して echonet-lite モジュールを初期化
// ECHONET Lite では通信はオブジェクト間で行われるため、通信元となる際のオブジェクトをここで指定します
var elsocket = EL.initialize([myObject], (rinfo, els, error) => {
  if (error) {
    console.log(error);
    return;
  }

  // 発見した ECHONET Lite 機器のIPアドレスと通信内容をダンプ
  console.log('====================');
  console.dir(rinfo, {depth: null});
  console.dir(els, {depth: null});
});

// ホームネットワーク上で ECHONET Lite 機器を探すコマンドを実行
EL.search();

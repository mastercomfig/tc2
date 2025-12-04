var u={exports:{}},n={};/**
 * @license React
 * react-jsx-runtime.production.js
 *
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */var l;function f(){if(l)return n;l=1;var e=Symbol.for("react.transitional.element"),c=Symbol.for("react.fragment");function o(d,r,t){var a=null;if(t!==void 0&&(a=""+t),r.key!==void 0&&(a=""+r.key),"key"in r){t={};for(var i in r)i!=="key"&&(t[i]=r[i])}else t=r;return r=t.ref,{$$typeof:e,type:d,key:a,ref:r!==void 0?r:null,props:t}}return n.Fragment=c,n.jsx=o,n.jsxs=o,n}var x;function m(){return x||(x=1,u.exports=f()),u.exports}var s=m();const h=[{name:"Game",href:"#",current:!0},{name:"Video",href:"#",current:!1},{name:"Audio",href:"#",current:!1},{name:"Keyboard / Mouse",href:"#",current:!1},{name:"Communication",href:"#",current:!1}];function p(...e){return e.filter(Boolean).join(" ")}function v(){return s.jsx("div",{children:s.jsx("div",{className:"block",children:s.jsx("nav",{"aria-label":"Tabs",className:"flex justify-center space-x-4 tf2-light uppercase text-2xl",children:h.map(e=>s.jsx("a",{href:e.href,"aria-current":e.current?"page":void 0,className:p(e.current?"bg-white/10 text-white":"text-gray-400 hover:text-white","rounded-md px-3 py-2 text-lg font-medium"),children:e.name},e.name))})})})}export{v as SettingsView};

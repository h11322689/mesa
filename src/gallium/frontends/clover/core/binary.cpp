   //
   // Copyright 2012 Francisco Jerez
   //
   // Permission is hereby granted, free of charge, to any person obtaining a
   // copy of this software and associated documentation files (the "Software"),
   // to deal in the Software without restriction, including without limitation
   // the rights to use, copy, modify, merge, publish, distribute, sublicense,
   // and/or sell copies of the Software, and to permit persons to whom the
   // Software is furnished to do so, subject to the following conditions:
   //
   // The above copyright notice and this permission notice shall be included in
   // all copies or substantial portions of the Software.
   //
   // THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   // IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   // FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   // THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
   // OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
   // ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
   // OTHER DEALINGS IN THE SOFTWARE.
   //
      #include <type_traits>
   #include <iostream>
      #include "core/binary.hpp"
      using namespace clover;
      namespace {
      template<typename T, typename = void>
            /// Serialize the specified object.
   template<typename T>
   void
   _proc(std::ostream &os, const T &x) {
                  /// Deserialize the specified object.
   template<typename T>
   void
   _proc(std::istream &is, T &x) {
                  template<typename T>
   T
   _proc(std::istream &is) {
      T x;
   _serializer<T>::proc(is, x);
               /// Calculate the size of the specified object.
   template<typename T>
   void
   _proc(binary::size_t &sz, const T &x) {
                  /// (De)serialize a scalar value.
   template<typename T>
   struct _serializer<T, typename std::enable_if<
            static void
   proc(std::ostream &os, const T &x) {
                  static void
   proc(std::istream &is, T &x) {
                  static void
   proc(binary::size_t &sz, const T &x) {
                     /// (De)serialize a vector.
   template<typename T>
   struct _serializer<std::vector<T>,
                  static void
   proc(std::ostream &os, const std::vector<T> &v) {
               for (size_t i = 0; i < v.size(); i++)
               static void
   proc(std::istream &is, std::vector<T> &v) {
               for (size_t i = 0; i < v.size(); i++)
               static void
   proc(binary::size_t &sz, const std::vector<T> &v) {
               for (size_t i = 0; i < v.size(); i++)
                  template<typename T>
   struct _serializer<std::vector<T>,
                  static void
   proc(std::ostream &os, const std::vector<T> &v) {
      _proc<uint32_t>(os, v.size());
   os.write(reinterpret_cast<const char *>(&v[0]),
               static void
   proc(std::istream &is, std::vector<T> &v) {
      v.resize(_proc<uint32_t>(is));
   is.read(reinterpret_cast<char *>(&v[0]),
               static void
   proc(binary::size_t &sz, const std::vector<T> &v) {
                     /// (De)serialize a string.
   template<>
   struct _serializer<std::string> {
      static void
   proc(std::ostream &os, const std::string &s) {
      _proc<uint32_t>(os, s.size());
               static void
   proc(std::istream &is, std::string &s) {
      s.resize(_proc<uint32_t>(is));
               static void
   proc(binary::size_t &sz, const std::string &s) {
                     /// (De)serialize a printf format
   template<>
   struct _serializer<binary::printf_info> {
      template<typename S, typename QT>
   static void
   proc(S & s, QT &x) {
      _proc(s, x.arg_sizes);
                  /// (De)serialize a binary::section.
   template<>
   struct _serializer<binary::section> {
      template<typename S, typename QT>
   static void
   proc(S &s, QT &x) {
      _proc(s, x.id);
   _proc(s, x.type);
   _proc(s, x.size);
                  /// (De)serialize a binary::argument.
   template<>
   struct _serializer<binary::argument> {
      template<typename S, typename QT>
   static void
   proc(S &s, QT &x) {
      _proc(s, x.type);
   _proc(s, x.size);
   _proc(s, x.target_size);
   _proc(s, x.target_align);
   _proc(s, x.ext_type);
                  /// (De)serialize a binary::symbol.
   template<>
   struct _serializer<binary::symbol> {
      template<typename S, typename QT>
   static void
   proc(S &s, QT &x) {
      _proc(s, x.name);
   _proc(s, x.attributes);
   _proc(s, x.reqd_work_group_size);
   _proc(s, x.section);
   _proc(s, x.offset);
                  /// (De)serialize a binary.
   template<>
   struct _serializer<binary> {
      template<typename S, typename QT>
   static void
   proc(S &s, QT &x) {
      _proc(s, x.syms);
   _proc(s, x.secs);
   _proc(s, x.printf_infos);
            };
      namespace clover {
      void
   binary::serialize(std::ostream &os) const {
                  binary
   binary::deserialize(std::istream &is) {
                  binary::size_t
   binary::size() const {
      size_t sz = 0;
   _proc(sz, *this);
         }

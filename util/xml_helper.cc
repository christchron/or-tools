// Copyright 2010-2011 Google
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <string>
#include "base/stringprintf.h"
#include "util/xml_helper.h"

namespace operations_research {

XmlHelper::XmlHelper() {}

void XmlHelper::StartDocument() {
  content_ = "<?xml version=\"1.0\"?>\n";
  direction_down_ = false;
}

void XmlHelper::StartElement(const string& name) {
  if (direction_down_) {
    content_.append(">\n");
  }
  tags_.push(name);
  StringAppendF(&content_, "<%s", name.c_str());
  direction_down_ = true;
}

void XmlHelper::AddAttribute(const string& key, int value) {
  AddAttribute(key, StringPrintf("%d", value));
}

void XmlHelper::AddAttribute(const string& key, const string& value) {
  std::ostringstream escaped_value;

  for (string::const_iterator it = value.begin(); it != value.end(); ++it) {
    unsigned char c = (unsigned char)*it;

    switch (c) {
      case '"':
        escaped_value << "&quot;";
        break;
      case '\'':
        escaped_value << "&apos;";
        break;
      case '<':
        escaped_value << "&lt;";
        break;
      case '>':
        escaped_value << "&gt;";
        break;
      case '&':
        escaped_value << "&amp;";
        break;
      default:
        escaped_value << c;
    }
  }

  StringAppendF(&content_, " %s=\"%s\"", key.c_str(),
                escaped_value.str().c_str());
}

void XmlHelper::EndElement() {
  string tag = tags_.top();

  if (direction_down_) {
    content_.append(" />\n");
  } else {
    StringAppendF(&content_, "</%s>\n", tag.c_str());
  }
  direction_down_ = false;

  tags_.pop();
}

void XmlHelper::EndDocument() {}

const string& XmlHelper::GetContent() const { return content_; }
}  // namespace

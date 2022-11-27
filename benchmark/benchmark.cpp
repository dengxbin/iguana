#include "iguana/json_reader.hpp"
#include "iguana/json_writer.hpp"
#include <chrono>
#include <iostream>
#ifdef HAS_RAPIDJSON
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#endif

class ScopedTimer {
public:
  ScopedTimer(const char *name)
      : m_name(name), m_beg(std::chrono::high_resolution_clock::now()) {}
  ScopedTimer(const char *name, uint64_t &ns) : ScopedTimer(name) {
    m_ns = &ns;
  }
  ~ScopedTimer() {
    auto end = std::chrono::high_resolution_clock::now();
    auto dur =
        std::chrono::duration_cast<std::chrono::nanoseconds>(end - m_beg);
    if (m_ns)
      *m_ns = dur.count();
    else
      std::cout << m_name << " : " << dur.count() << " ns\n";
  }

private:
  const char *m_name;
  std::chrono::time_point<std::chrono::high_resolution_clock> m_beg;
  uint64_t *m_ns = nullptr;
};

inline constexpr std::string_view json0 = R"(
{
   "fixed_name_object": {
      "name0": "James",
      "name1": "Abraham",
      "name2": "Susan",
      "name3": "Frank",
      "name4": "Alicia"
   },
   "another_object": {
      "string": "here is some text",
      "another_string": "Hello World",
      "boolean": false,
      "nested_object": {
         "v3s": [[0.12345, 0.23456, 0.001345],
                  [0.3894675, 97.39827, 297.92387],
                  [18.18, 87.289, 2988.298]],
         "id": "298728949872"
      }
   },
   "string_array": ["Cat", "Dog", "Elephant", "Tiger"],
   "string": "Hello world",
   "number": 3.14,
   "boolean": true,
   "another_bool": false
}
)";

std::string read_json(const std::string &filename) {
  std::error_code ec;
  uint64_t size = std::filesystem::file_size(filename, ec);
  if (ec) {
    throw std::runtime_error("file size error " + ec.message());
  }

  if (size == 0) {
    throw std::runtime_error("empty file");
  }
  std::ifstream file(filename, std::ios::binary);
  std::string content;
  content.resize(size);

  file.read(content.data(), size);
  return content;
}

const auto cur_file_path =
    std::filesystem::path(__FILE__).parent_path().string();

std::string json_numbers = read_json(cur_file_path + "/../data/numbers.json");
std::vector<double> create_numbers_obj() {
  std::vector<double> numbers;
  iguana::from_json(numbers, std::begin(json_numbers), std::end(json_numbers));
  return numbers;
}

struct fixed_name_object_t {
  std::string name0{};
  std::string name1{};
  std::string name2{};
  std::string name3{};
  std::string name4{};

#ifdef HAS_RAPIDJSON
  template <typename Writer> void Serialize(Writer &writer) const {
    writer.StartObject();
    writer.String("name0");
    writer.String(name0);
    writer.String("name1");
    writer.String(name1);
    writer.String("name2");
    writer.String(name2);
    writer.String("name3");
    writer.String(name3);
    writer.String("name4");
    writer.String(name4);
    writer.EndObject();
  }
#endif
};
REFLECTION(fixed_name_object_t, name0, name1, name2, name3, name4);

struct nested_object_t {
  std::vector<std::array<double, 3>> v3s{};
  std::string id{};

#ifdef HAS_RAPIDJSON
  template <typename Writer> void Serialize(Writer &writer) const {
    writer.StartObject();
    writer.String("v3s");
    writer.StartArray();
    for (auto &arr : v3s) {
      writer.StartArray();
      for (auto &d : arr) {
        writer.Double(d);
      }

      writer.EndArray();
    }

    writer.EndArray();
    writer.String("id");
    writer.String(id);
    writer.EndObject();
  }
#endif
};
REFLECTION(nested_object_t, v3s, id);

struct another_object_t {
  std::string string{};
  std::string another_string{};
  bool boolean{};
  nested_object_t nested_object{};

#ifdef HAS_RAPIDJSON
  template <typename Writer> void Serialize(Writer &writer) const {
    writer.StartObject();
    writer.String("string");
    writer.String(string);
    writer.String("another_string");
    writer.String(another_string);
    writer.String("boolean");
    writer.Bool(boolean);
    writer.String("nested_object");
    nested_object.Serialize(writer);
    writer.EndObject();
  }
#endif
};
REFLECTION(another_object_t, string, another_string, boolean, nested_object);

struct obj_t {
  //   fixed_object_t fixed_object{};
  fixed_name_object_t fixed_name_object{};
  another_object_t another_object{};
  std::vector<std::string> string_array{};
  std::string string{};
  double number{};
  bool boolean{};
  bool another_bool{};

#ifdef HAS_RAPIDJSON
  template <typename Writer> void Serialize(Writer &writer) const {
    writer.StartObject();
    writer.String("fixed_name_object");
    fixed_name_object.Serialize(writer);
    writer.String("another_object");
    another_object.Serialize(writer);
    writer.String("string_array");
    writer.StartArray();
    for (auto &str : string_array) {
      writer.String(str);
    }
    writer.EndArray();
    writer.String("string");
    writer.String(string);

    writer.String("number");
    writer.Double(number);

    writer.String("boolean");
    writer.Bool(boolean);

    writer.String("another_bool");
    writer.Bool(another_bool);

    writer.EndObject();
  }
#endif
};
REFLECTION(obj_t, fixed_name_object, another_object, string_array, string,
           number, boolean, another_bool);

obj_t create_object() {
  fixed_name_object_t fix_obj = {"James", "Abraham", "Susan", "Frank",
                                 "Alicia"};

  nested_object_t nested_obj = {{{0.12345, 0.23456, 0.001345},
                                 {0.3894675, 97.39827, 297.92387},
                                 {18.18, 87.289, 2988.298}},
                                "298728949872"};

  another_object_t another_obj = {"here is some text", "Hello World", false,
                                  nested_obj};

  obj_t obj = {fix_obj,       another_obj, {"Cat", "Dog", "Elephant", "Tiger"},
               "Hello world", 3.14,        true,
               false};
  return obj;
}

constexpr int iterations = 100000;

void test_from_json() {
  obj_t obj;
  iguana::from_json(obj, std::begin(json0), std::end(json0));

  {
    ScopedTimer timer("iguana   parse  json");
    for (int i = 0; i < iterations; ++i) {
      iguana::from_json(obj, std::begin(json0), std::end(json0));
    }
  }

#ifdef HAS_RAPIDJSON
  rapidjson::Document doc;
  doc.Parse(json0.data(), json0.size());

  {
    ScopedTimer timer("rapidjson parse json");
    for (int i = 0; i < iterations; ++i) {
      doc = {};
      doc.Parse(json0.data(), json0.size());
    }
  }
#endif
}

void test_to_json() {
  obj_t obj = create_object();

  iguana::string_stream ss;
  iguana::to_json(obj, ss);

  {
    ScopedTimer timer("iguana   to  json");
    for (int i = 0; i < iterations; ++i) {
      ss.clear();
      iguana::to_json(obj, ss);
    }
  }

#ifdef HAS_RAPIDJSON
  rapidjson::StringBuffer sb;
  rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
  obj.Serialize(writer);

  {
    ScopedTimer timer("rapidjson to json");
    for (int i = 0; i < iterations; ++i) {
      sb.Clear();
      writer.Reset(sb);
      obj.Serialize(writer);
    }
  }
#endif
}

void test_from_json_numbers() {
  std::vector<double> numbers;
  iguana::from_json(numbers, std::begin(json_numbers), std::end(json_numbers));

  {
    ScopedTimer timer("iguana    parse numbers json");
    for (int i = 0; i < iterations; ++i) {
      iguana::from_json(numbers, std::begin(json_numbers),
                        std::end(json_numbers));
    }
  }

#ifdef HAS_RAPIDJSON
  rapidjson::Document doc;
  doc.Parse(json_numbers.data(), json_numbers.size());

  {
    ScopedTimer timer("rapidjson parse numbers json");
    for (int i = 0; i < iterations; ++i) {
      doc = {};
      doc.Parse(json_numbers.data(), json_numbers.size());
    }
  }
#endif
}

void test_to_json_numbers() {
  const auto obj = create_numbers_obj();

  iguana::string_stream ss;
  iguana::to_json(obj, ss);

  {
    ScopedTimer timer("iguana    numbers objs to  json");
    for (int i = 0; i < iterations; ++i) {
      ss.clear();
      iguana::to_json(obj, ss);
    }
  }

#ifdef HAS_RAPIDJSON
  rapidjson::StringBuffer sb;
  rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
  writer.StartObject();
  writer.StartArray();
  for (auto &num : obj) {
    writer.Double(num);
  }
  writer.EndArray();
  writer.EndObject();

  {
    ScopedTimer timer("rapidjson  numbers objs to json");
    for (int i = 0; i < iterations; ++i) {
      sb.Clear();
      writer.Reset(sb);
      writer.StartObject();
      writer.StartArray();
      for (auto &num : obj) {
        writer.Double(num);
      }
      writer.EndArray();
      writer.EndObject();
    }
  }
#endif
}

int main() {
  for (int i = 0; i < 10; ++i) {
    test_to_json();
    std::cout << "====================\n";
  }

  for (int i = 0; i < 10; ++i) {
    test_from_json();
    std::cout << "====================\n";
  }

  for (int i = 0; i < 10; ++i) {
    test_from_json_numbers();
    std::cout << "====================\n";
  }
  for (int i = 0; i < 10; ++i) {
    test_to_json_numbers();
    std::cout << "====================\n";
  }
}
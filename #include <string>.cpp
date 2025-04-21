#include <string>
#include <unordered_map>

class Object
{
  private:
    std::unordered_map<std::string, std::string> map_;

  public:
    void insert(std::string key, std::string value)
    {
        map_[std::move(key)] = std::move(value);
    }

    std::string get(const std::string &key) const
    {
        auto it = map_.find(key);
        return (it != map_.end()) ? it->second : "";
    }
};

int main()
{
    Object obj;
    obj.insert("name", "Alice");
    obj.insert("age", "30");

    std::cout << obj.get("name"); // Alice
    std::cout << obj.get("age"); // 30
    std::cout << obj.get("city"); // (пустая строка)
}

#pragma once

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <list>
#include <string_view>
#include <algorithm>
#include <optional>
#include <vector>
#include <cmath>
#include <iostream>
namespace svg {

using Color = std::string;
inline const Color NoneColor{"none"};

class ObjectContainer;
class Drawable;

enum class StrokeLineCap {
    BUTT,
    ROUND,
    SQUARE,
};

enum class StrokeLineJoin {
    ARCS,
    BEVEL,
    MITER,
    MITER_CLIP,
    ROUND,
};

template <typename Owner>
class PathProps {
public:
    Owner& SetFillColor(Color color) {
        fill_color_ = std::move(color);
        return AsOwner();
    }
    Owner& SetStrokeColor(Color color) {
        stroke_color_ = std::move(color);
        return AsOwner();
    }

    Owner& SetStrokeWidth(double width){
         stroke_width_ = width;
         return AsOwner();
    }

    Owner& SetStrokeLineCap(StrokeLineCap line_cap){
        stroke_line_cap_str_ = ToStringStrokeLineCap(line_cap);
        return AsOwner();
    }

    Owner& SetStrokeLineJoin(StrokeLineJoin line_join){
          stroke_line_join_str_ = ToStringStrokeLineJoin(line_join);
          return AsOwner();
    }

protected:
    ~PathProps() = default;

    void RenderAttrs(std::ostream& out) const {
        using namespace std::literals;

        if (fill_color_) {
            out << " fill=\""sv << *fill_color_ << "\""sv;
        }
        if (stroke_color_) {
            out << " stroke=\""sv << *stroke_color_ << "\""sv;
        }
        if (stroke_width_) {
             out << " stroke-width=\""sv << *stroke_width_ << "\""sv;
        }
        if (stroke_line_cap_str_){
             out << " stroke-linecap=\""sv << *stroke_line_cap_str_ << "\""sv;
        }
        if (stroke_line_join_str_){
             out << " stroke-linejoin=\""sv << *stroke_line_join_str_<< "\""sv;
        }

    }

private:
    inline const char* ToStringStrokeLineCap(StrokeLineCap v)
    {
        switch (v)
        {
            case StrokeLineCap::BUTT:    return "butt";
            case StrokeLineCap::ROUND:   return "round";
            case StrokeLineCap::SQUARE:  return "square";
            default:      return "";
        }
    }

    inline const char* ToStringStrokeLineJoin(StrokeLineJoin v)
    {
        switch (v)
        {
            case StrokeLineJoin::ARCS:          return "arcs";
            case StrokeLineJoin::BEVEL:         return "bevel";
            case StrokeLineJoin::MITER:         return "miter";
            case StrokeLineJoin::MITER_CLIP:    return "miter-clip";
            case StrokeLineJoin::ROUND:         return "round";
            default:            return "";
        }
    }

    Owner& AsOwner() {
        // static_cast безопасно преобразует *this к Owner&,
        // если класс Owner — наследник PathProps
        return static_cast<Owner&>(*this);
    }

    std::optional<Color> fill_color_;
    std::optional<Color> stroke_color_;
    std::optional<double>stroke_width_;
    std::optional<std::string> stroke_line_cap_str_;
    std::optional<std::string> stroke_line_join_str_;
};


struct Point {
    Point() = default;
    Point(double x, double y)
        : x(x)
        , y(y) {
    }
    double x = 0;
    double y = 0;
};

/*
 * Вспомогательная структура, хранящая контекст для вывода SVG-документа с отступами.
 * Хранит ссылку на поток вывода, текущее значение и шаг отступа при выводе элемента
 */
struct RenderContext {
    RenderContext(std::ostream& out)
        : out(out) {
    }

    RenderContext(std::ostream& out, int indent_step, int indent = 0)
        : out(out)
        , indent_step(indent_step)
        , indent(indent) {
    }

    RenderContext Indented() const {
        return {out, indent_step, indent + indent_step};
    }

    void RenderIndent() const {
        for (int i = 0; i < indent; ++i) {
            out.put(' ');
        }
    }

    std::ostream& out;
    int indent_step = 0;
    int indent = 0;
};

/*
 * Абстрактный базовый класс Object служит для унифицированного хранения
 * конкретных тегов SVG-документа
 * Реализует паттерн "Шаблонный метод" для вывода содержимого тега
 */
class Object{
public:
    virtual void Render(const RenderContext& context) const;

    virtual ~Object() = default;

private:
    virtual void RenderObject(const RenderContext& context) const = 0;
};

/*
 * Класс Circle моделирует элемент <circle> для отображения круга
 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/circle
 */

class Circle final : public Object , public PathProps<Circle> {
public:
    Circle& SetCenter(Point center);
    Circle& SetRadius(double radius);
    void Render(const RenderContext& context) const override;

private:
    void RenderObject(const RenderContext& context) const override;

    Point center_;
    double radius_ = 1.0;
};

/*
 * Класс Polyline моделирует элемент <polyline> для отображения ломаных линий
 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/polyline
 */
class Polyline final : public Object ,public PathProps<Polyline>  {
public:
    // Добавляет очередную вершину к ломаной линии
    Polyline& AddPoint(Point point);
    void Render(const RenderContext& context) const override;

    /*
     * Прочие методы и данные, необходимые для реализации элемента <polyline>
     */
private:
    void RenderObject(const RenderContext& context) const override;

    std::list<Point>points_poly;
};

/*
 * Класс Text моделирует элемент <text> для отображения текста
 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/text
 */
class Text final : public Object,public PathProps<Text> {
public:
    // Задаёт координаты опорной точки (атрибуты x и y)
    Text& SetPosition(Point pos);

    // Задаёт смещение относительно опорной точки (атрибуты dx, dy)
    Text& SetOffset(Point offset);

    // Задаёт размеры шрифта (атрибут font-size)
    Text& SetFontSize(uint32_t size);

    // Задаёт название шрифта (атрибут font-family)
    Text& SetFontFamily(std::string font_family);

    // Задаёт толщину шрифта (атрибут font-weight)
    Text& SetFontWeight(std::string font_weight);

    // Задаёт текстовое содержимое объекта (отображается внутри тега text)
    Text& SetData(std::string data);

   void Render(const RenderContext& context) const override;

    // Прочие данные и методы, необходимые для реализации элемента <text>
private:
    void RenderObject(const RenderContext& context) const override;
    std::string EncodingForXML(std::string text);
    Point position_{};
    Point offset_{};
    uint32_t size_font_ =1;
    std::string font_name_;
    std::string font_bold_;
    std::string label_;
};

class Drawable {
  public:
    virtual void Draw(ObjectContainer& objs) const = 0;
    virtual ~Drawable() = default;
};

class ObjectContainer {
  public:
    virtual void AddPtr(std::unique_ptr<Object>&& obj) = 0;

    template <typename Object>
     void Add(Object obj) {
            figures_.emplace_back(std::make_unique<Object>(std::move(obj)));
     }
  protected:
   ~ObjectContainer() =default;
    std::list<std::unique_ptr<Object>> figures_;
};

class Document final :public ObjectContainer {
public:

    void AddPtr(std::unique_ptr<Object>&& obj) override;

    // Выводит в ostream svg-представление документа
    void Render(std::ostream& out) const;

private :
     ObjectContainer *obj_;
  //   std::list<std::unique_ptr<Object>> figures_;




};

}  // namespace svg

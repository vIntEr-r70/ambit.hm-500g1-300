#pragma once

#include <numeric>

template <typename Source>
class ArcItemCmp
{
    typedef std::pair<double, double> Point2;
    typedef std::vector<Point2> Points;

public:

    ArcItemCmp(Source f, Source s) noexcept
        : f_(f), s_(s)
    {
        calcLinearInterpolation();
    }

    void init(Source f, Source s) noexcept
    {
        f_ = f; s_ = s;
        calcLinearInterpolation();
    }

    bool update() noexcept {
        return calcLinearInterpolation();
    }

public:

    // Абсолютная ошибка
    double absError() const noexcept
    {
        // Это максимальная абсолютная разница между двумя измерениями в одно время
        return std::accumulate(results_.begin(), results_.end(), 0.0,
            [](double pmax, Point2 const& points) {
                return std::max(pmax, points.second);
            });
    }

    // Средняя квадратичная ошибка
    double midSquareError() const noexcept
    {
        double const sum = std::accumulate(results_.begin(), results_.end(), 0.0,
            [] (double pmax,  Point2 const& points) {
                return pmax + std::pow(points.second, 2);
            });
        return std::sqrt(sum) / (results_.size() - 1);
    }

    // Интегральная ошибка
    double integralError() const noexcept
    {
        Points f;
        for (std::size_t idx = 0, size = f_.size(); idx < size; ++idx)
            f.emplace_back(f_.key(idx), f_.value(idx));

        double const p1 = integral(f);
        double const p_err = integral(results_);
        return p_err / p1 * 100.0;
    }

private:

    // вычисление интеграла
    double integral(Points const& points) const noexcept
    {
        double sum = 0.0;

        auto fit = points.begin();
        if (fit != points.end())
        {
            auto sit = std::next(fit);
            while (sit != points.end())
            {
                double const h = std::abs(fit->first - sit->first);
                sum += h * (fit->second + sit->second) / 2.0;

                fit = sit;
                sit = std::next(sit);
            }
        }

        return sum;
    }

    bool calcLinearInterpolation() noexcept
    {
        struct Helper
        {
            static double xFirst(Source const& i) noexcept
            {
                return i.size() ? i.key(0) : 0.0;
            }

            static double xLast(Source const& i) noexcept
            {
                std::size_t const size = i.size();
                return size ? i.key(size - 1) : 0.0;
            }

            static double interp(double const key, Source const& i) noexcept
            {
                // Находим точку конца отрезка, в который попадает наша точка
                std::size_t const size = i.size();

                std::size_t fit = 0;
                if (fit == size) return 0.0;

                std::size_t sit = fit + 1;
                while (sit != size && !(key >= i.key(fit) && key <= i.key(sit)))
                {
                    fit = sit;
                    sit += 1;
                }

                if (sit == size)
                    return 0.0;

                Point2 pf = std::make_pair(i.key(fit), i.value(fit));
                Point2 ps = std::make_pair(i.key(sit), i.value(sit));

                if (std::fabs(ps.first - pf.first) <= std::numeric_limits<double>::epsilon())
                    return ps.second;

                double const k = (ps.second - pf.second) / (ps.first - pf.first);
                double const b = pf.second - k * pf.first;

                return k * key + b;
            }
        };

        // Количество точек, попадающих в общую область
        std::size_t const n = std::min(f_.size(), s_.size());
        if (n == 0) return false;

        // Минимальное и максимальное значение пересечения
        double const pToX = std::min(Helper::xLast(f_), Helper::xLast(s_));
        double const pFromX = std::max(Helper::xFirst(f_), Helper::xFirst(s_));
        double const pStep = (pToX - pFromX) / static_cast<double>(n - 1);

        // Рассчитываем общие координаты
        for (double point = pFromX; point < pToX; point += pStep)
        {
            double const fi = Helper::interp(point, f_);
            double const si = Helper::interp(point, s_);
            results_.emplace_back(point, std::fabs(fi - si));
        }
        // Так-же рассчитываем последнее значение
        results_.emplace_back(pToX, std::fabs(Helper::interp(pToX, f_) - Helper::interp(pToX, s_)));
        return true;
    }

private:

    Source f_;
    Source s_;

    Points results_;
};

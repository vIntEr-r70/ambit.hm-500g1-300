#include <iostream>

#include <vector>
#include <numeric>
#include <cmath>
#include <cassert>
#include <fstream>

using namespace std;

class Source
{
public:

    typedef std::pair<double, double> Point2;
    typedef std::vector<Source::Point2> Points;

public:

    Source(char const* const fName) noexcept
    {
        // Загружаем данные из файла
        std::ifstream fs(fName, std::ios::in | std::ios::binary);

        std::string line;
        while (std::getline(fs, line))
        {
            points_.resize(points_.size() + 1);
            std::sscanf(line.c_str(), "%lf\t%lf", &points_.back().first, &points_.back().second);
        }
    }

public:

    std::size_t size() const noexcept { return points_.size(); }

    double xFirst() const noexcept { return points_.empty() ? 0.0 : points_.front().first; }

    double xLast() const noexcept { return points_.empty() ? 0.0 : points_.back().first; }

    double interp(double const point) const noexcept
    {
        // Находим точку конца отрезка, в который попадает наша точка
		auto fit = points_.begin();		
		if (fit == points_.end())
			return 0.0;

		auto sit = std::next(fit);
		while (sit != points_.end() && !(point >= fit->first && point <= sit->first))
		{
			fit = sit;
			sit = std::next(sit);
		}

		if (sit == points_.end())
			return 0.0;

        Point2 pf = *fit;
		Point2 ps = *sit;

        if (std::fabs(ps.first - pf.first) <= std::numeric_limits<double>::epsilon())
			return ps.second;

        double const k = (ps.second - pf.second) / (ps.first - pf.first);
        double const b = pf.second - k * pf.first;

        return k * point + b;
    }

    Points const& points() const noexcept { return points_; }

private:

    Points points_;
};


class Calculator
{
public:

    Calculator(Source const& f, Source const& s) noexcept
        : f_(f), s_(s)
    {
        calcLinearInterpolation(f_, s_);
    }

public:

    // Абсолютная ошибка
    double absError() const noexcept
    {
        // Это максимальная абсолютная разница между двумя измерениями в одно время
        return std::accumulate(results_.begin(), results_.end(), 0.0,
            [](double pmax, Source::Point2 const& points) {
                return std::max(pmax, points.second);
            });
    }

    // Средняя квадратичная ошибка
    double midSquareError() const noexcept
    {
        double const sum = std::accumulate(results_.begin(), results_.end(), 0.0,
            [] (double pmax, Source::Point2 const& points) {
                return pmax + std::pow(points.second, 2);
            });
        return std::sqrt(sum) / (results_.size() - 1);
    }

    // Интегральная ошибка
    double integralError() const noexcept
    {
        double const p1 = integral(f_.points());
        double const p_err = integral(results_);		
        return p_err / p1 * 100.0;
    }

private:

    // вычисление интеграла
    double integral(Source::Points const& points) const noexcept
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

    void calcLinearInterpolation(Source const& f, Source const& s) noexcept
    {
        // Количество точек, попадающих в общую область
        std::size_t const n = std::min(f.size(), s.size());

        // Минимальное и максимальное значение пересечения
        double const pToX = std::min(f.xLast(), s.xLast());
        double const pFromX = std::max(f.xFirst(), s.xFirst());
        double const pStep = (pToX - pFromX) / static_cast<double>(n - 1);

        // Рассчитываем общие координаты
        for (double point = pFromX; point < pToX; point += pStep)
		{
			double const fi = f.interp(point);
			double const si = s.interp(point);
			results_.emplace_back(point, std::fabs(fi - si));
		}
		// Так-же рассчитываем последнее значение
		results_.emplace_back(pToX, std::fabs(f.interp(pToX) - s.interp(pToX)));
    }

private:

    Source const& f_;
    Source const& s_;

    Source::Points results_;
};



int main()
{
    Source f("00-ДТ1, °C");
    Source s("00-ДТ2, °C");
    Calculator c(f, s);

    std::cout << "absError = " << c.absError() << std::endl;
    std::cout << "midSquareError = " << c.midSquareError() << std::endl;
    std::cout << "integralError = " << c.integralError() << std::endl;

    return 0;
}

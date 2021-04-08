#include <iostream>
#include <stack>

int findMin(const int *courses, const int &k)
{
    int min = courses[0];
    int index = 0;
    for (int i = 1; i < k; ++i)
    {
        if (courses[i] < min)
        {
            min = courses[i];
            index = i;
        }
    }
    return index;
}

int raft(const int *arr, const int &n, const int &k)
{
    std::stack<int> arrValues;
    for (int i = 0; i < n; ++i)
    {
        arrValues.push(arr[i]);
    }

    int *courses = new int[k];
    for (int i = 0; i < k; ++i)
    {
        courses[i] = arrValues.top();
        arrValues.pop();
    }
    while (!arrValues.empty())
    {
        courses[findMin(courses, k)] += arrValues.top();
        arrValues.pop();
    }

    int max = courses[0];

    for (int i = 1; i < k; ++i)
    {
        if (max < courses[i])
        {
            max = courses[i];
        }
    }

    delete courses;
    return max;
}

void sort(int *arr, const int &n)
{
    for (int i = 1; i < n; ++i)
    {
        int key = arr[i];
        int j = i - 1;
        while (key < arr[j] && j >= 0)
        {
            arr[j + 1] = arr[j];
            --j;
        }
        arr[j + 1] = key;
    }
}

int main()
{
    int n, k;
    std::cin >> n >> k;

    int *arr = new int[n];

    for (int i = 0; i < n; ++i)
    {
        std::cin >> arr[i];
    }

    sort(arr, n);

    std::cout << raft(arr, n, k);

    delete arr;
    return 0;
}
#include <iostream>
#include <vector>

bool isPossible(const int *arr, const int &n, const int &k, const int &raft)
{
    std::vector<int> weights;
    for (int i = 0; i < n; ++i)
    {
        weights.push_back(arr[i]);
    }
    for (int i = 0; i < k; ++i)
    {
        int sum = 0;
        int index = weights.size() - 1;
        while (index >= 0)
        {
            if (sum + weights[index] <= raft)
            {
                sum += weights[index];
                weights.erase(weights.begin() + index);
            }
            --index;
        }
    }
    return weights.empty();
}

int raft(const int *arr, const int &n, const int &k)
{
    if (k < 1)
    {
        return 0;
    }

    int sum = 0;
    for (int i = 0; i < n; ++i)
    {
        sum += arr[i];
    }
    if (k == 1)
    {
        return sum;
    }
    if (n < k)
    {
        return arr[n - 1];
    }
    int raft = std::max(sum / k, arr[n - 1]);

    while (raft < sum && !isPossible(arr, n, k, raft))
    {
        ++raft;
    }
    return raft;
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
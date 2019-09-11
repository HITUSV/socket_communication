/******************************************************************************
 *
 * Copyright 2019 wumo1999@gmail.com
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *****************************************************************************/

//
// Created by wumode on 2019/8/8.
//

#ifndef SOCKET_COMMUNICATION_CALLBACK_FUNCTION_H
#define SOCKET_COMMUNICATION_CALLBACK_FUNCTION_H

#include <functional>
#include <cstring>
#include <iostream>
#include <vector>
#include <memory>
#include <map>

class CallBackFunctionInterface{};

template<typename D, typename C>
class CallBackFunction:public CallBackFunctionInterface{
public:
    CallBackFunction()= default;
    ~CallBackFunction()= default;
    virtual bool Conversion(const C* c, D* d){
        memcpy(d, c, sizeof(D));
    };
    template <typename callable, typename ...A>
    const std::function<void(C)>* SetFunction(callable&& fun, A&&... arg) {
        task_ = std::bind(std::forward<callable>(fun), std::placeholders::_1, std::forward<A>(arg)...);
        public_task_ = std::bind([this](C buffer){
            D data;
            if(!Conversion(&buffer, &data)){
                return;
            };
            task_(&data);}
            , std::placeholders::_1);
        return &public_task_;
    }

private:
    std::function<void(const D*)> task_;
    std::function<void(C)> public_task_;
};

template <typename... C>
class CallbackPair{
public:
    ~CallbackPair() = default;
    CallbackPair(const std::shared_ptr<CallBackFunctionInterface>& c, const std::function<void(C...)>* t){
        task = t;
        callback_function_ptr = c;
    };
    std::shared_ptr<CallBackFunctionInterface> callback_function_ptr;
    const std::function<void(C...)>* task;
};

class CallBackFunctionWithoutData:public CallBackFunctionInterface{
public:
    CallBackFunctionWithoutData()= default;
    ~CallBackFunctionWithoutData()= default;
    template <typename callable, typename... A>
    const std::function<void(void)>* SetFunction(callable&& fun, A&&... arg) {
        task_ = std::bind(std::forward<callable>(fun), std::forward<A>(arg)...);
        public_task_ = std::bind([this](){task_();});
        return &public_task_;
    }

private:
    std::function<void(void)> task_;
    std::function<void(void)> public_task_;
};

#endif //SOCKET_COMMUNICATION_CALLBACK_FUNCTION_H

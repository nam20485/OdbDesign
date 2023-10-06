from django.shortcuts import render
from django.http import HttpResponse
from PyOdbDesignLib import PyOdbDesignLib


def index(request):
    net = PyOdbDesignLib.Net("net1", 0)
    return HttpResponse("Hello, world. You're at the api index. net name: " + net.GetName() + " net id: " + str(net.GetIndex()))
